#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cc.h"
#include "cache.h"
#include "bus.h"

// CACHE FUNCTIONS
Cache* cache_create(uint cache_size, uint assoc, uint block_size) {
    Cache* c = malloc(sizeof(Cache));

    uint col_size = cache_size / assoc;
    uint num_sets = col_size / block_size;

    c->num_sets = num_sets;
    c->assoc = assoc;
    c->block_size = block_size;

    c->idx_bits = 31 - __builtin_clz(num_sets);
    c->off_bits = 31 - __builtin_clz(block_size);
    c->idx_off_bits = 31 - __builtin_clz(col_size);
    c->idx_ones = c->idx_bits ? 0xFFFFFFFF >> (32 - c->idx_bits) : 0;
    // ternary op. to handle idx_bits == 0 because >> by 32 is undefined                                                     

    LOG("idx_bits: %d\n", c->idx_bits);
    LOG("off_bits: %d\n", c->off_bits);
    LOG("idx_off_bits: %d\n", c->idx_off_bits);
    LOG("\n");

    // malloc
    size_t total_entries = num_sets * assoc;
    c->tags = malloc(total_entries * sizeof(uint));
    c->states = malloc(total_entries * sizeof(uint));
    c->ts_used = malloc(total_entries * sizeof(uint));

    c->t_next = 1;
    memset(c->states, 0, total_entries * sizeof(uint));
    memset(c->ts_used, 0, total_entries * sizeof(uint));

    c->total_cycles = 0;
    c->comp_cycles = 0;
    c->p_hits = 0;
    c->s_hits = 0;
    c->misses = 0;

    return c;
}

void cache_free(Cache* c) {
    if (!c) return;

    free(c->tags);
    free(c->states);
    free(c->ts_used);
    free(c);
}

// instruction 2 (compute)
void comp(Cache* c, uint cycles) {
    c->total_cycles += cycles;
    c->comp_cycles += cycles;
}

// instruction 0 (read)
void cache_pr_rd (Cache* c, Bus* b, uint addr) {
    ulong cycles = 0;

    int has_match = 0;
    uint tag = addr >> c->idx_off_bits;
    uint idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("read  address: %x\n", addr);
    LOG("tag: %x\n", tag);
    LOG("idx: %x\n", idx);

    uint way = 0; // way to be updated
    uint lru_t = UINT_MAX;

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    for (uint j = 0; j < c->assoc; ++j) {
        uint arr_j = idx * c->assoc + j;
        LOG("%d,%d: tag: %x  state: %d\n", idx, j, c->tags[arr_j], c->states[arr_j]);
        if (c->tags[arr_j] == tag && c->states[arr_j]) {
            // cache hit
            LOG("wow cache hit\n");
            has_match = 1;
            way = j;
            if (c->states[arr_j] == ST_S) {
                ++c->s_hits; 
            } else {
                ++c->p_hits;
            }
            break;
        }

        // track lru way; free lines (state 0: empty or invalidated by coherence)
        // count as oldest, so they are filled before evicting a valid line
        uint ts = c->states[arr_j] ? c->ts_used[arr_j] : 0;
        if (ts < lru_t) {
            lru_t = ts;
            way = j;
        }
    }

    uint arr_i = idx * c->assoc + way;

    // 2. (if no match) evict/load block
    if (!has_match) {
        LOG("oh no cache miss\n");
        ++c->misses;

        if (c->states[arr_i] == ST_M) bus_flush_mem(c, b);

        // LOAD new block
        c->tags[arr_i] = tag;
        c->states[arr_i] = bus_rd(c, b, addr);
    }

    LOG("way: %d\n\n", way);

    // 3. update t
    c->ts_used[arr_i] = c->t_next;
    ++c->t_next;

    c->total_cycles += cycles;
}

// instruction 1 (write)
void cache_pr_wr (Cache* c, Bus* b, uint addr) {
    ulong cycles = 0;

    int has_match = 0;
    uint tag = addr >> c->idx_off_bits;
    uint idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("write address: %x\n", addr);
    LOG("tag: %x\n", tag);
    LOG("idx: %x\n", idx);

    uint way = 0; // way to be updated
    uint lru_t = UINT_MAX;

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    for (uint j = 0; j < c->assoc; ++j) {
        uint arr_j = idx * c->assoc + j;
        LOG("%d,%d: tag: %x  state: %d\n", idx, j, c->tags[arr_j], c->states[arr_j]);
        if (c->tags[arr_j] == tag && c->states[arr_j]) {
            // cache hit
            LOG("wow cache hit\n");
            has_match = 1;
            way = j;
            if (c->states[arr_j] == ST_S) {
                ++c->s_hits;
                bus_rdx(c, b, addr);       // S -> M: invalidate others (BusRdX)
            } else {
                ++c->p_hits;               // E or M: silent
            }
            break;
        }

        // track lru way; free lines (state 0: empty or invalidated by coherence)
        // count as oldest, so they are filled before evicting a valid line
        uint ts = c->states[arr_j] ? c->ts_used[arr_j] : 0;
        if (ts < lru_t) {
            lru_t = ts;
            way = j;
        }
    }

    uint arr_i = idx * c->assoc + way;

    // 2. (if no match) evict/load block
    if (!has_match) {
        LOG("oh no cache miss\n");
        ++c->misses;

        // if block is dirty 
        if (c->states[arr_i] == ST_M) bus_flush_mem(c, b);
        
        // LOAD new block
        bus_rdx(c, b, addr);
        c->tags[arr_i] = tag;
    }

    LOG("way: %d\n\n", way);

    // 3. for store instructions: update state to dirty
    c->states[arr_i] = ST_M;

    // 4. update t
    c->ts_used[arr_i] = c->t_next;
    ++c->t_next;

    c->total_cycles += cycles;
}

// check if cache contains a certain address
int cache_find(Cache* c, uint addr) {
    uint tag = addr >> c->idx_off_bits;
    uint idx = (addr >> c->off_bits) & c->idx_ones;
    for (uint j = 0; j < c->assoc; ++j) {
        uint arr_j = idx * c->assoc + j;
        if (c->tags[arr_j] == tag && c->states[arr_j]) return arr_j;
    }
    return -1;
}

// returns 1 if a miss, or a write to a cache line with state S
int cache_needs_bus(Cache* c, uint addr, int is_write) {
    int k = cache_find(c, addr);
    if (k < 0) return 1;
    return is_write && c->states[k] == ST_S;
}

