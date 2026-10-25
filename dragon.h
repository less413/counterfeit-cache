#ifndef DRAGON_H
#define DRAGON_H

#include <stdint.h>

#include "cache.h"
#include "bus.h"
#include "protocol.h"

// cache functions
void dragon_cache_pr_rd(Cache *c, Bus *b, uint32_t addr);
void dragon_cache_pr_wr(Cache *c, Bus *b, uint32_t addr);
int dragon_cache_needs_bus(Cache *c, uint32_t addr, int is_write);

// bus functions


static const Protocol DRAGON_PROTOCOL = {dragon_cache_pr_rd, dragon_cache_pr_wr, dragon_cache_needs_bus};

#endif

