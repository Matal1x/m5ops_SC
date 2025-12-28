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

    uint64_t lvl_cached = 0, lvl_far = 0;
    uint64_t cached = measure_access_time_with_level(&array[0], &lvl_cached);
    uint64_t cold   = measure_access_time_with_level(&array[array_size - 64], &lvl_far);

    printf("Cached access: %lu cycles, hit level = %lu\n",
           (unsigned long)cached, (unsigned long)lvl_cached);
    printf("Far (likely uncached) access: %lu cycles, hit level = %lu\n",
           (unsigned long)cold, (unsigned long)lvl_far);

    free(array);
    return 0;
}
