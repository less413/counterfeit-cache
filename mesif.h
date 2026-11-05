#ifndef MESIF_H
#define MESIF_H

#include <stdint.h>

#include "cache.h"
#include "bus.h"
#include "protocol.h"

// 0 = invalid, 1 = shared, 2 = exclusive, 3 = modified, 5 = forward
enum { MESIF_I = 0, MESIF_S = 1, MESIF_E = 2, MESIF_M = 3, MESIF_F = 5 };

// cache functions
void mesif_cache_pr_rd(Cache *c, Bus *b, uint32_t addr);
void mesif_cache_pr_wr(Cache *c, Bus *b, uint32_t addr);
int mesif_cache_needs_bus(Cache *c, uint32_t addr, int is_write);

int mesif_cache_bus_rd(Cache *c, uint32_t addr);
int mesif_cache_bus_rdx(Cache *c, uint32_t addr);

// bus functions
uint32_t mesif_bus_rd(Cache *c, Bus *b, uint32_t addr);
void mesif_bus_rdx(Cache *c, Bus *b, uint32_t addr);

static const Protocol MESIF_PROTOCOL = {mesif_cache_pr_rd, mesif_cache_pr_wr, mesif_cache_needs_bus};

#endif

