#include <stdint.h>

#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "dragon.h"

// instruction 0 (read)
void dragon_cache_pr_rd(Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;

    LOG("\nREAD  address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // TODO
}

// instruction 1 (write)
void dragon_cache_pr_wr(Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;

    LOG("\nWRITE address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // TODO
}

int dragon_cache_needs_bus(Cache* c, uint32_t addr, int is_write) {
    LOG("\ncache_needs_bus check\n");

    // TODO
    return 0;
}

