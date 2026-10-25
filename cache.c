#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cc.h"
#include "cache.h"

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

// check if cache contains a certain address
// returns cache block on hit, victim block on miss
CacheBlock* cache_get_block(Cache* c, uint32_t addr, int* hit) {
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
CacheBlock* cache_find(Cache* c, uint32_t addr) {
    int hit;
    CacheBlock* block = cache_get_block(c, addr, &hit);
    return hit ? block : NULL;
}
