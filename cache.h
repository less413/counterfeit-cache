#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

typedef struct Bus Bus;

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
    uint32_t* tags;
    uint32_t* states;
    uint32_t* ts_used;

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
void cache_pr_rd(Cache* c, Bus* b, uint32_t addr);
void cache_pr_wr(Cache* c, Bus* b, uint32_t addr);

int cache_bus_rd(Cache* c, uint32_t addr);
int cache_bus_rdx(Cache* c, uint32_t addr);

int cache_find(Cache* c, uint32_t addr);
int cache_needs_bus(Cache* c, uint32_t addr, int is_write);

#endif
