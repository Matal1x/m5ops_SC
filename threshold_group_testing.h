#ifndef THRESHOLD_GROUP_TESTING_H
#define THRESHOLD_GROUP_TESTING_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uintptr_t *addresses;
    void *backing; // this points to the allocated memory pool
    size_t size;
    size_t capacity;
} address_set_t;


typedef struct {
    size_t associativity;      
    size_t cache_line_size;    
    size_t page_size;    
    size_t l2_size;      
} cache_config_t;

typedef struct {
    void *target_address;
    void *calibration_data;    // Timing thresholds (cache_timing_t*)
    int skip_calibration;
    const char *calibration_file;
} test_context_t;


address_set_t create_address_set(size_t capacity);
void free_address_set(address_set_t *set);
void print_address_set(const address_set_t *set, const char *name);


// Function pointer type for testing if a set is an eviction set
typedef int (*eviction_test_func_t)(const address_set_t *set, 
                                   const test_context_t *context);


address_set_t threshold_group_reduction(const address_set_t *candidate_set,
                                       const cache_config_t *config,
                                       eviction_test_func_t test_func,
                                       const test_context_t *context);



#endif //THRESHOLD_GROUP_TESTING_H