#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

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

CacheBlock* cache_get_block(Cache* c, uint32_t addr, int* hit);
CacheBlock* cache_find(Cache* c, uint32_t addr);

#endif
