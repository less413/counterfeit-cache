#ifndef DRAGON_H
#define DRAGON_H

#include <stdint.h>

#include "cache.h"
#include "bus.h"
#include "protocol.h"

// 0 = invalid, 1 = shared clean, 3 = shared modified, 5 = exclusive, 7 = modified
// before changing this note that some functions perform bitwise ops on these numbers
enum { D_I = 0, D_SC = 1, D_SM = 3, D_E = 5, D_M = 7 };

// cache functions
void dragon_cache_pr_rd(Cache *c, Bus *b, uint32_t addr);
void dragon_cache_pr_wr(Cache *c, Bus *b, uint32_t addr);
int dragon_cache_needs_bus(Cache *c, uint32_t addr, int is_write);

int dragon_cache_bus_rd(Cache* c, uint32_t addr);
int dragon_cache_bus_upd(Cache* c, uint32_t addr);

// bus functions
int dragon_bus_rd(Cache *c, Bus *b, uint32_t addr);
int dragon_bus_upd(Cache *c, Bus *b, uint32_t addr);
// might make a combined BusRd + BusUpd function in the future for shared write misses

static const Protocol DRAGON_PROTOCOL = {dragon_cache_pr_rd, dragon_cache_pr_wr, dragon_cache_needs_bus};

#endif

