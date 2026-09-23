struct Bus;

// CACHE
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

typedef struct Bus {
    // bus info (none for now)
    // may store refs to all 4 Caches in future

    // bus stats
    ulong data_traffic;
    ulong invalidations;
} Bus;


Cache* cache_create(uint cache_size, uint assoc, uint block_size);
void cache_free(Cache* c);

Bus* bus_create();
void bus_free(Bus* b);

void comp(Cache* c, uint cycles);
void cache_pr_rd(Cache* c, Bus* b, uint addr);
void cache_pr_wr(Cache* c, Bus* b, uint addr);


void bus_flush_mem(Cache* c, Bus* b);
void bus_rd(Cache* c, Bus* b);

