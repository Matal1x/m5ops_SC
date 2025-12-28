#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <x86intrin.h>


static inline uint64_t rdtsc() {
    return __rdtsc();
}

static inline void memory_fence() {
    _mm_mfence();
}

static inline void compiler_barrier() {
    asm volatile("" ::: "memory");
}

uint64_t measure_access_time(volatile void *address);

#endif