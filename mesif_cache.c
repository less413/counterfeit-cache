#include <stdint.h>

#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "mesif.h"

// instruction 0 (read)
void mesif_cache_pr_rd(Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;
    // uint32_t idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("\nREAD  address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        if (victim->state == MESIF_S || victim->state == MESIF_F) {
            ++c->s_hits; 
        } else {
            ++c->p_hits;
        }
    } else { // miss
        ++c->misses;

        if (victim->state == MESIF_M) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        victim->tag = tag;
        victim->state = mesif_bus_rd(c, b, addr);
    }

    // 3. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

// instruction 1 (write)
void mesif_cache_pr_wr(Cache* c, Bus* b, uint32_t addr) {
    uint32_t tag = addr >> c->idx_off_bits;
    // uint32_t idx = (addr >> c->off_bits) & c->idx_ones;

    LOG("\nWRITE address: %x\n", addr);

    c->total_cycles += ACCESS_CYCLES;

    // 1. check set for match
    int hit;
    CacheBlock* victim = cache_get_block(c, addr, &hit);

    // 2. handle hit/miss
    if (hit) {
        if (victim->state == MESIF_S || victim->state == MESIF_F) {
            ++c->s_hits; 
            mesif_bus_rdx(c, b, addr); // S/F -> M: invalidate others (BusRdX)
        } else {
            ++c->p_hits;
        }
    } else { // miss
        ++c->misses;

        // if block is dirty 
        if (victim->state == MESIF_M) {
            bus_flush_mem(c, b);
        }

        // LOAD new block
        mesif_bus_rdx(c, b, addr);
        victim->tag = tag;
    }

    // 3. for store instructions: update state to dirty
    victim->state = MESIF_M;

    // 4. update t
    victim->t_used = c->t_next;
    ++c->t_next;
}

// returns 1 if a miss, or a write to a cache line with state S/F
int mesif_cache_needs_bus(Cache* c, uint32_t addr, int is_write) {
    LOG("\ncache_needs_bus check\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return 1;
    }
    return is_write && (block->state == MESIF_S || block->state == MESIF_F);
}

// BusRd:  E/S/M/F -> S   (M flushes; memory picks it up too)
// returns the state before the snoop (MESIF_I if not present)
int mesif_cache_bus_rd(Cache* c, uint32_t addr) {
    LOG("\nbus_read\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return MESIF_I;
    }
    int old = block->state;
    block->state = MESIF_S;
    return old;
}

// BusRdX: E/S/M/F -> I   (M flushes; memory picks it up too)
// returns the state before the snoop (MESIF_I if not present)
int mesif_cache_bus_rdx(Cache* c, uint32_t addr) {
    LOG("\nbus_rdx\n");
    CacheBlock* block = cache_find(c, addr);
    if (block == NULL) {
        return MESIF_I;
    }
    int old = block->state;
    block->state = MESIF_I;
    return old;
}

