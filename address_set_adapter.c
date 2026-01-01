#include "address_set_adapter.h"

#include <gem5/m5ops.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- m5-op eviction oracle ----
static int m5op_eviction_test(const address_set_t *set,
                              const test_context_t *context)
{
    volatile uint8_t *target = (volatile uint8_t *)context->target_address;
    if (!target) return 0;

    volatile uint8_t tmp = *target;
    (void)tmp;
    asm volatile("" ::: "memory");

    for (size_t i = 0; i < set->size; i++) {
        volatile uint8_t *p = (volatile uint8_t *)set->addresses[i];
        tmp = *p;
    }
    (void)tmp;
    asm volatile("" ::: "memory");

    tmp = *target;
    (void)tmp;

    uint64_t lvl = m5_get_last_hit_level();
    return (lvl == 3); // 3 => miss/evicted, 1/2 => hit
}

eviction_test_func_t create_eviction_tester(void)
{
    return m5op_eviction_test;
}

// ---- Candidate generation (your original code) ----
address_set_t generate_candidate_set(void *target_addr,
                                     size_t num_candidates,
                                     const cache_config_t *cfg)
{
    size_t l2_size = cfg->l2_size;
    size_t num_sets = l2_size / (cfg->cache_line_size * cfg->associativity);
    size_t stride = num_sets * cfg->cache_line_size;

    const size_t total_bytes = num_candidates * stride;
    uint8_t *pool = (uint8_t*)aligned_alloc(cfg->cache_line_size, total_bytes);
    if (!pool) {
        perror("aligned_alloc failed");
        exit(1);
    }

    memset(pool, 0xA5, total_bytes);

    uintptr_t base = (uintptr_t)target_addr;
    uintptr_t base_index_bits = base & (stride - 1);

    address_set_t set = create_address_set(num_candidates);
    set.backing = pool;
    set.size = num_candidates;

    for (size_t i = 0; i < num_candidates; i++) {
        uintptr_t addr = (uintptr_t)pool + (i * stride);
        addr = (addr & ~(stride - 1)) | base_index_bits;
        set.addresses[i] = addr;
    }

    // Shuffle candidates
    for (size_t i = num_candidates - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        uintptr_t tmp = set.addresses[i];
        set.addresses[i] = set.addresses[j];
        set.addresses[j] = tmp;
    }

    printf("[CandidateGen] %zu candidates generated for target %p (stride=%zu bytes)\n",
           num_candidates, target_addr, stride);

    return set;
}
