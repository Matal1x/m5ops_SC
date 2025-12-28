#include "timing.h"
#include <stdio.h>

uint64_t measure_access_time(volatile void *address) {
    volatile uint64_t *addr = (volatile uint64_t*)address;
    
    // Memory fence to prevent out-of-order execution
    memory_fence();
    compiler_barrier();
    
    uint64_t start = rdtsc();
    // Force the compiler to actually perform the memory access
    volatile uint64_t value = *addr;
    (void)value; // Prevent unused variable warning
    uint64_t end = rdtsc();
    
    memory_fence();
    compiler_barrier();
    
    return end - start;
}