#ifndef CACHE_H
#define CACHE_H

#include <sys/types.h>

typedef struct Bus Bus;

typedef struct Cache {
    // cache info
    uint num_sets;
    uint assoc;
    uint block_size;

    // helpers
    uint idx_bits; // this might be redundant
    uint off_bits;
    uint idx_off_bits;
    uint idx_ones;

    // entries
    uint t_next;
    uint* tags;
    uint* states;
    uint* ts_used;

    // stats
    ulong total_cycles;
    ulong comp_cycles;
    ulong p_hits;
    ulong s_hits;
    ulong misses;
} Cache;

Cache* cache_create(uint cache_size, uint assoc, uint block_size);
void cache_free(Cache* c);

void comp(Cache* c, uint cycles);
void cache_pr_rd(Cache* c, Bus* b, uint addr);
void cache_pr_wr(Cache* c, Bus* b, uint addr);

int cache_find(Cache* c, uint addr);
int cache_needs_bus(Cache* c, uint addr, int is_write);

#endif
