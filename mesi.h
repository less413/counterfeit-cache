#ifndef MESI_H
#define MESI_H

#include <stdint.h>

#include "cache.h"
#include "bus.h"

// 0 = invalid, 1 = shared, 2 = exclusive, 3 = modified
enum { ST_I = 0, ST_S = 1, ST_E = 2, ST_M = 3 };

// cache functions
void mesi_cache_pr_rd(Cache *c, Bus *b, uint32_t addr);
void mesi_cache_pr_wr(Cache *c, Bus *b, uint32_t addr);
int mesi_cache_needs_bus(Cache *c, uint32_t addr, int is_write);

int mesi_cache_bus_rd(Cache *c, uint32_t addr);
int mesi_cache_bus_rdx(Cache *c, uint32_t addr);

// bus functions
uint32_t mesi_bus_rd(Cache *c, Bus *b, uint32_t addr);
void mesi_bus_rdx(Cache *c, Bus *b, uint32_t addr);

#endif
