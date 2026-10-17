#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "cc.h"
#include "cache.h"

struct Bus {
    // bus info
    // store refs to all 4 Caches
    Cache* caches[NUM_CORES];

    // bus stats
    uint64_t data_traffic;
    uint64_t invalidations;
    uint64_t t_free; // cycle at which the bus is next free
};

Bus* bus_create(Cache** caches);
void bus_free(Bus* b);
void bus_flush_mem(Cache* c, Bus* b);
uint32_t bus_rd(Cache* c, Bus* b, uint32_t addr);
void bus_rdx(Cache* c, Bus* b, uint32_t addr);

#endif
