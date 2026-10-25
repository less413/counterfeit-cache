#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <limits.h>

typedef struct Bus Bus;

typedef struct CacheBlock {
    uint32_t tag;
    uint32_t state;
    uint32_t t_used;
} CacheBlock;

typedef struct Cache {
    // cache info
    uint32_t num_sets;
    uint32_t assoc;
    uint32_t block_size;

    // helpers
    uint32_t idx_bits; // this might be redundant
    uint32_t off_bits;
    uint32_t idx_off_bits;
    uint32_t idx_ones;

    // entries
    uint32_t t_next;
    uint32_t* blocks;

    // stats
    uint64_t total_cycles;
    uint64_t comp_cycles;
    uint64_t p_hits;
    uint64_t s_hits;
    uint64_t misses;
} Cache;

Cache* cache_create(uint32_t cache_size, uint32_t assoc, uint32_t block_size);
void cache_free(Cache* c);

void comp(Cache* c, uint32_t cycles);

// check if cache contains a certain address
// returns cache block on hit, LRU victim block on miss
inline CacheBlock* cache_get_block(Cache* c, uint32_t addr, int* hit) {
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
inline CacheBlock* cache_find(Cache* c, uint32_t addr) {
    int hit;
    CacheBlock* block = cache_get_block(c, addr, &hit);
    return hit ? block : NULL;
}

#endif
