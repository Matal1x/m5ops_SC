#include "threshold_group_testing.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

address_set_t create_address_set(size_t capacity) {
    address_set_t set;
    set.addresses = malloc(capacity * sizeof(uintptr_t));
    set.backing   = NULL;
    set.size = 0;
    set.capacity = capacity;
    return set;
}


void free_address_set(address_set_t *set) {
    if (set && set->addresses) {
        free(set->addresses);
        set->addresses = NULL;
        if (set->backing) {
            free(set->backing);
            set->backing = NULL;
        }
        set->size = 0;
        set->capacity = 0;
    }
}


void print_address_set(const address_set_t *set, const char *name) {
    printf("%s: size=%zu, capacity=%zu\n", name, set->size, set->capacity);
    for (size_t i = 0; i < set->size; i++) {
        printf("  [%zu] 0x%lx\n", i, set->addresses[i]);
    }
}

/* 
* Partition the set S into (p+1) disjoint subsets of approximately equal size
* @S: Pointer to the set to partition
* @subsets: Pointer to the subsets array
* p: Equals to the cache associativity. Number (+1) of subsets to be created
*/
static void partition_set(const address_set_t *S, address_set_t *subsets, size_t p) {
    size_t subset_size = S->size / (p + 1);
    size_t remainder = S->size % (p + 1);
    
    size_t current_idx = 0;
    for (size_t i = 0; i < p + 1; i++) {
        
        size_t this_subset_size = subset_size + (i < remainder ? 1 : 0);
        
        
        subsets[i].addresses = &S->addresses[current_idx];
        subsets[i].size = this_subset_size;
        subsets[i].capacity = this_subset_size; 
        
        current_idx += this_subset_size;
    }
}


/* 
* Create a new set that is S without the subset T_j
* @S: Pointer to the set to reduce
* @subsets: Pointer to the subsets array
* @num_subsets: 
* @exclude_index: Index of the subset to remove
*/
static address_set_t create_set_without_subset(const address_set_t *S, 
                                              const address_set_t *subsets, 
                                              size_t num_subsets,
                                              size_t exclude_index) {
    
    size_t new_size = S->size - subsets[exclude_index].size;
    address_set_t new_set = create_address_set(new_size);
    
    
    size_t idx = 0;
    for (size_t i = 0; i < num_subsets; i++) {
        if (i != exclude_index) {
            memcpy(&new_set.addresses[idx], 
                   subsets[i].addresses, 
                   subsets[i].size * sizeof(uintptr_t));
            idx += subsets[i].size;
        }
    }
    new_set.size = new_size;
    
    return new_set;
}

// Main threshold group testing algorithm
address_set_t threshold_group_reduction(const address_set_t *candidate_set,
                                       const cache_config_t *config,
                                       eviction_test_func_t test_func,
                                       const test_context_t *context) {
    
    // Create a working copy of the candidate set
    address_set_t S = create_address_set(candidate_set->size);
    memcpy(S.addresses, candidate_set->addresses, candidate_set->size * sizeof(uintptr_t));
    S.size = candidate_set->size;
    
    // Result set (minimal eviction set)
    address_set_t result = create_address_set(config->associativity);
    
    size_t a = config->associativity;
    
    printf("Starting threshold group reduction:\n");
    printf("  Initial set size: %zu\n", S.size);
    printf("  Target associativity: %zu\n", a);
    printf("  Target address: 0x%lx\n", (uintptr_t)context->target_address);
    
    const int MAX_RETRIES_SAME_SIZE = 3;   // number of retries at same size
    int retry = 0;

    while (S.size > a) {
        printf("Reducing from %zu to ", S.size);

        // Create a+1 disjoint subsets
        address_set_t *subsets = malloc((a + 1) * sizeof(address_set_t));
        partition_set(&S, subsets, a);

        // Try to find a subset that can be safely removed
        int found_reducible_subset = 0;

        for (size_t j = 0; j < a + 1; j++) {
            // Create S without T_j
            address_set_t S_without_Tj =
                create_set_without_subset(&S, subsets, a + 1, j);

            // Test if S without T_j is still an eviction set
            if (test_func(&S_without_Tj, context)) {
                printf("%zu elements (removed subset %zu)\n",
                       S_without_Tj.size, j);

                // Replace S with S_without_Tj
                free_address_set(&S);
                S = S_without_Tj;

                found_reducible_subset = 1;
                retry = 0;  // reset retry counter after success
                break;
            } else {
                // Not an eviction set without this subset, so keep looking
                free_address_set(&S_without_Tj);
            }
        }

        // Free subsets array (note: we don't free the addresses since they point into S)
        free(subsets);

        if (!found_reducible_subset) {
            if (S.size > a && retry < MAX_RETRIES_SAME_SIZE) {
                retry++;
                printf("No reducible subset found at size %zu "
                       "(retry %d/%d)... waiting a bit\n",
                       S.size, retry, MAX_RETRIES_SAME_SIZE);
                fflush(stdout);
                usleep(50000);  // wait 50ms before retrying
                continue;       // retry at same size
            }

            printf("FAILED - cannot find reducible subset\n");
            break;
        }
    }
    
    // We now have a minimal eviction set (or we failed)
    if (S.size == a) {
        result.size = S.size;
        memcpy(result.addresses, S.addresses, S.size * sizeof(uintptr_t));
        printf("SUCCESS: Found minimal eviction set of size %zu\n", result.size);
    } else {
        printf("FAILED: Could not reduce to minimal size. Current size: %zu\n", S.size);
        // Return whatever we have
        result.size = S.size;
        memcpy(result.addresses, S.addresses, S.size * sizeof(uintptr_t));
    }
    
    free_address_set(&S);
    return result;
}