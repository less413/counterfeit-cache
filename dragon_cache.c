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

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        // do nothing!
        if (victim->state == D_SC || victim->state == D_SM) {
            ++c->s_hits; 
        } else {
            ++c->p_hits;
        }
    } else { // miss
        ++c->misses;

        // M/Sm states get written to mem if evicted
        if (victim->state == D_M || victim->state == D_SM) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        victim->tag = tag;
        // if other copies exist: this -> Sc, otherwise -> E
        victim->state = dragon_bus_rd(c, b, addr) ? D_SC : D_E;
    }

    // 3. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

// instruction 1 (write)
void dragon_cache_pr_wr(Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;

    LOG("\nWRITE address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        // send out BusUpd if shared hit.
        if (victim->state == D_SC || victim->state == D_SM) {
            ++c->s_hits; 
            // this -> Sm or M, depending on whether other copies exist
            victim->state = dragon_bus_upd(c, b, addr) ? D_SM : D_M;
        } else {
            ++c->p_hits;
            // E or M: this -> M and do nothing else
            victim->state = D_M;
        }
    } else { // miss
        ++c->misses;

        // M/Sm states get written to mem if evicted
        if (victim->state == D_M || victim->state == D_SM) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        victim->tag = tag;
        if (dragon_bus_rd(c, b, addr)) {
            // shared: also send BusUpd signal
            dragon_bus_upd(c, b, addr);
            victim->state = D_SM;
        } else {
            victim->state = D_M;
        }
    }

    // 3. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

int dragon_cache_needs_bus(Cache* c, uint32_t addr, int is_write) {
    LOG("\ncache_needs_bus check\n");

    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        // all misses need bus
        return 1;
    }
    // write shared hit
    // TODO funny bitwise??
    return is_write && (block->state == D_SC || block->state == D_SM);
}

// returns state before snoop, or D_I if absent
int dragon_cache_bus_rd(Cache* c, uint32_t addr) {
    LOG("\nbus_read\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return D_I;
    }
    int old = block->state;
    // E -> Sc, M -> Sm (and Sc -> Sc, Sm -> Sm)
    block->state &= 3;
    return old;
}

// returns state before snoop, or D_I if absent
int dragon_cache_bus_upd(Cache* c, uint32_t addr) {
    LOG("\nbus_update\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return D_I;
    }
    int old = block->state;
    // Sm -> Sc (and Sc -> Sc. E and M should never receive this but -> Sc anyway)
    block->state = D_SC;
    return old;
}

