#ifndef BUS_H
#define BUS_H

#include <sys/types.h>
#include "cc.h"
#include "cache.h"

struct Bus {
    // bus info
    // store refs to all 4 Caches
    Cache* caches[NUM_CORES];

    // bus stats
    ulong data_traffic;
    ulong invalidations;
};

Bus* bus_create(Cache** caches);
void bus_free(Bus* b);
void bus_flush_mem(Cache* c, Bus* b);
void bus_rd(Cache* c, Bus* b);

#endif
