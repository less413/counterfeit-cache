typedef struct {
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
    ulong p_hits;
    ulong s_hits;
    ulong misses;
} Cache;

Cache* cache_create(uint cache_size, uint assoc, uint block_size);
void cache_free(Cache* c);

ulong cache_pr_rd (Cache* c, uint addr);
ulong cache_pr_wr (Cache* c, uint addr);

