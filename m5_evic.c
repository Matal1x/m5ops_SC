#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "threshold_group_testing.h"
#include "address_set_adapter.h"

static address_set_t create_tmp_set_from_array(uintptr_t *addrs, size_t n)
{
    address_set_t tmp = create_address_set(n);
    tmp.backing = NULL;
    tmp.size = n;
    for (size_t i = 0; i < n; ++i) tmp.addresses[i] = addrs[i];
    return tmp;
}

int main(int argc, char **argv)
{
    unsigned seed = 12345;
    int tries = 5;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tries") == 0 && i + 1 < argc) {
            tries = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = (unsigned)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--tries <N>] [--seed <S>]\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    srand(seed);

    cache_config_t cfg = {
        .associativity   = 8,
        .cache_line_size = 64,
        .page_size       = 4096,
        .l2_size         = 256 * 1024
    };

    // Context: only target_address is used by the m5-op oracle
    test_context_t ctx = {0};

    uint8_t *target = (uint8_t*)aligned_alloc(cfg.cache_line_size, 64);
    if (!target) {
        perror("aligned_alloc");
        return 1;
    }
    memset(target, 0xAB, 64);
    ctx.target_address = target;

    eviction_test_func_t oracle = create_eviction_tester();

    // Sanity: empty set should NOT evict (expect 0)
    address_set_t empty = create_address_set(1);
    empty.size = 0;
    printf("Empty-set eviction (expect 0): %d\n", oracle(&empty, &ctx));
    free_address_set(&empty);

    int evicts = 0;
    address_set_t candidates = (address_set_t){0};

    int nb_candidate = 128;
    do {
        if (candidates.addresses)
            free_address_set(&candidates);

        printf("\n=== Step 1: Generate candidate addresses ===\n");
        candidates = generate_candidate_set(target, nb_candidate, &cfg);
        printf("Generated %zu candidates.\n", candidates.size);

        printf("\n=== Step 2: Test candidate pool (m5-op oracle) ===\n");
        evicts = oracle(&candidates, &ctx);
        printf("Candidate pool eviction: %s\n", evicts ? "✅ YES (evicts)" : "❌ NO (does not evict)");

        if (!evicts) {
            printf("Initial candidate set does not evict, doubling candidates.\n");
            nb_candidate *= 2;
            tries--;
        }
    } while (!evicts && tries > 0);

    if (!evicts) {
        printf("Could not find an initial eviction set. Increase candidates/stride.\n");
        free_address_set(&candidates);
        free(target);
        return -1;
    }

    printf("\n=== Step 3: Threshold Group Reduction ===\n");
    address_set_t minimal = threshold_group_reduction(&candidates, &cfg, oracle, &ctx);

    printf("\nReduction complete.\n");
    printf("  Original size: %zu\n", candidates.size);
    printf("  Minimal set size: %zu\n", minimal.size);

    printf("\n=== Step 4: Verification ===\n");
    int still_evicts = oracle(&minimal, &ctx);
    printf("Verification: %s\n", still_evicts ? "✅ Still evicts" : "❌ No longer evicts");

    printf("\n=== Step 5: Leave-One-Out minimality test ===\n");
    uintptr_t *min_addrs = (uintptr_t*)malloc(minimal.size * sizeof(uintptr_t));
    if (!min_addrs) { perror("malloc min_addrs"); goto cleanup; }
    for (size_t i = 0; i < minimal.size; ++i) min_addrs[i] = minimal.addresses[i];

    for (size_t k = 0; k < minimal.size; ++k) {
        uintptr_t *arr = (uintptr_t*)malloc((minimal.size - 1) * sizeof(uintptr_t));
        if (!arr) { perror("malloc arr"); break; }

        for (size_t i = 0, j = 0; i < minimal.size; ++i) {
            if (i == k) continue;
            arr[j++] = min_addrs[i];
        }

        address_set_t tmp = create_tmp_set_from_array(arr, minimal.size - 1);
        int still = oracle(&tmp, &ctx);
        printf("Remove idx %zu -> %s\n", k, still ? "still evicts (BAD) ❌" : "no longer evicts (OK) ✅");
        free_address_set(&tmp);
        free(arr);
    }
    free(min_addrs);

    printf("\n=== Step 6: Dump eviction set ===\n");
    {
        FILE *f = fopen("evset_dump.txt", "w");
        if (!f) {
            perror("fopen evset_dump.txt");
        } else {
            size_t num_sets = cfg.l2_size / (cfg.associativity * cfg.cache_line_size);
            size_t stride = num_sets * cfg.cache_line_size;
            size_t pool_size = candidates.size * stride;

            fprintf(f, "# seed=%u\n", seed);
            fprintf(f, "# l2_size=%zu\n", cfg.l2_size);
            fprintf(f, "# associativity=%zu\n", cfg.associativity);
            fprintf(f, "# cache_line=%zu\n", cfg.cache_line_size);
            fprintf(f, "# page_size=%zu\n", cfg.page_size);
            fprintf(f, "# stride=%zu\n", stride);
            fprintf(f, "# candidate_pool_size=%zu\n", candidates.size);
            fprintf(f, "# pool_size=%zu\n", pool_size);
            fprintf(f, "# minimal_count=%zu\n", minimal.size);
            fprintf(f, "# offsets (hex)  then virtual addresses\n");

            if (candidates.backing) {
                uintptr_t pool_base = (uintptr_t)candidates.backing;
                for (size_t i = 0; i < minimal.size; ++i) {
                    uintptr_t off = minimal.addresses[i] - pool_base;
                    fprintf(f, "0x%zx  0x%lx\n", off, (unsigned long)minimal.addresses[i]);
                }
            } else {
                for (size_t i = 0; i < minimal.size; ++i)
                    fprintf(f, "0x%lx\n", (unsigned long)minimal.addresses[i]);
            }
            fclose(f);
            printf("Dumped minimal eviction set to evset_dump.txt\n");
        }
    }

cleanup:
    free_address_set(&minimal);
    free_address_set(&candidates);
    free(target);
    printf("\n=== Done ===\n");
    return 0;
}
