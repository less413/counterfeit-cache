#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cc.h"
#include "cache.h"
#include "bus.h"

static CacheBlock* cache_get_block(Cache* c, uint32_t addr, int* hit);
static CacheBlock* cache_find(Cache* c, uint32_t addr);

Cache* cache_create(uint32_t cache_size, uint32_t assoc, uint32_t block_size) {
    Cache* c = malloc(sizeof(Cache));

    uint32_t col_size = cache_size / assoc;
    uint32_t num_sets = col_size / block_size;

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
    c->blocks = malloc(total_entries * sizeof(CacheBlock));
    memset(c->blocks, 0, total_entries * sizeof(CacheBlock));

    c->t_next = 1;
    c->total_cycles = 0;
    c->comp_cycles = 0;
    c->p_hits = 0;
    c->s_hits = 0;
    c->misses = 0;

    return c;
}

void cache_free(Cache* c) {
    if (!c) return;

    free(c->blocks);
    free(c);
}

// instruction 2 (compute)
void comp(Cache* c, uint32_t cycles) {
    c->total_cycles += cycles;
    c->comp_cycles += cycles;
}

// instruction 0 (read)
void cache_pr_rd (Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;
    // uint32_t idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("\nREAD  address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        if (victim->state == ST_S) {
            ++c->s_hits; 
        } else {
            ++c->p_hits;
        }
    } else { // miss
        ++c->misses;

        if (victim->state == ST_M) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        victim->tag = tag;
        victim->state = bus_rd(c, b, addr);
    }

    // 3. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

// instruction 1 (write)
void cache_pr_wr (Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;
    // uint32_t idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("\nWRITE address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        if (victim->state == ST_S) {
            ++c->s_hits; 
            bus_rdx(c, b, addr); // S -> M: invalidate others (BusRdX)
        } else {
            ++c->p_hits;
        }
    } else { // miss
        ++c->misses;

        // if block is dirty 
        if (victim->state == ST_M) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        bus_rdx(c, b, addr);
        victim->tag = tag;
    }

    // 3. for store instructions: update state to dirty
    victim->state = ST_M;

    // 4. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

// BusRd:  E/S/M -> S   (M flushes; memory picks it up too)
// returns 1 if sends data, 0 otherwise
int cache_bus_rd(Cache* c, uint32_t addr) {
    LOG("\nbus_read\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return 0;
    }
    block->state = ST_S;
    return 1;
}

// BusRdX: E/S/M -> I   (int exclusive flag to indicate BusRdX)
// returns 1 if sends (and flushes) data, 0 otherwise
int cache_bus_rdx(Cache* c, uint32_t addr) {
    LOG("\nbus_rdx\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return 0;
    }
    block->state = ST_I;
    return 1;
}

// check if cache contains a certain address
// returns cache block on hit, victim block on miss
static CacheBlock* cache_get_block(Cache* c, uint32_t addr, int* hit) {
    uint32_t tag = addr >> c->idx_off_bits;
    uint32_t idx = (addr >> c->off_bits) & c->idx_ones;

    CacheBlock* base = (CacheBlock*) c->blocks + (idx * c->assoc);
    CacheBlock* victim = base; // way to be updated
    uint32_t lru_t = UINT_MAX;

    for (uint32_t j = 0; j < c->assoc; ++j) {
        CacheBlock* curr = base + j;
        LOG("%d,%d: tag: %x  state: %d\n", idx, j, curr->tag, curr->state);
        if (curr->tag == tag && curr->state) {
            // cache hit
            LOG("wow cache hit\n");
            *hit = 1;
            return curr;
        }

        // track lru way; free lines (state 0: empty or invalidated by coherence)
        // count as oldest, so they are filled before evicting a valid line
        uint32_t ts = curr->state ? curr->t_used : 0;
        if (ts < lru_t) {
            lru_t = ts;
            victim = curr;
        }
    }

    LOG("oh no cache miss\n");
    *hit = 0;
    return victim;
}


// check if cache contains a certain address
// returns cache block on hit, NULL on miss
static CacheBlock* cache_find(Cache* c, uint32_t addr) {
    int hit;
    CacheBlock* block = cache_get_block(c, addr, &hit);
    return hit ? block : NULL;
}

// returns 1 if a miss, or a write to a cache line with state S
int cache_needs_bus(Cache* c, uint32_t addr, int is_write) {
    LOG("\ncache_needs_bus check\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return 1;
    }
    return is_write && block->state == ST_S;
}

