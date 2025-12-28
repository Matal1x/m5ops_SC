#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <gem5/m5ops.h>
#include "timing.h"

int main() {
    const size_t array_size = 1 << 20; // 1 MB
    uint8_t *array = aligned_alloc(64, array_size);
    if (!array) {
        perror("alloc failed");
        return 1;
    }

    // Touch every page to ensure mapping
    for (size_t i = 0; i < array_size; i += 4096)
        array[i] = 1;

    // Warm up first line (make it cached)
    for (int i = 0; i < 1000; i++) {
        volatile uint8_t tmp = array[0];
        (void)tmp;
    }

    // Measure cached access
    uint64_t cached = measure_access_time(&array[0]);
    uint64_t cached_lvl = m5_get_last_hit_level();

    // Access a far away location (likely not in cache)
    uint64_t cold = measure_access_time(&array[array_size - 64]);
    uint64_t cold_lvl = m5_get_last_hit_level();

    printf("Cached access: %lu cycles, hit level = %lu\n",
           cached, cached_lvl);
    printf("Far (likely uncached) access: %lu cycles, hit level = %lu\n",
           cold, cold_lvl);

    free(array);
    return 0;
}
