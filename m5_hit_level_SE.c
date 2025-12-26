#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <gem5/m5ops.h>

#ifndef CACHELINE
#define CACHELINE 64
#endif

static inline void touch_byte(volatile uint8_t *p)
{
    // A volatile load that the compiler cannot remove.
    (void)*p;
}

static void thrash(volatile uint8_t *buf, size_t size)
{
    // Touch one byte per cache line to create pressure in the cache.
    for (size_t off = 0; off < size; off += CACHELINE) {
        touch_byte(buf + off);
    }
}

static void print_level(const char *tag)
{
    uint64_t level = m5_get_last_hit_level();
    // Your encoding: 0=None, 1=L1_HIT, 2=L2_HIT, 3=MEM_HIT
    printf("%s: last_hit_level=%lu\n", tag, (unsigned long)level);
}

int main(void)
{
    // Choose sizes relative to your config:
    // L1D = 32 KiB, L2 = 1 MiB (from your setup)
    const size_t L1_THRASH = 128 * 1024;   // > 32 KiB to evict from L1
    const size_t L2_THRASH = 4 * 1024 * 1024; // > 1 MiB to evict from L2 (likely)

    volatile uint8_t *target = (volatile uint8_t*)aligned_alloc(CACHELINE, CACHELINE);
    volatile uint8_t *thrash_l1 = (volatile uint8_t*)aligned_alloc(CACHELINE, L1_THRASH);
    volatile uint8_t *thrash_l2 = (volatile uint8_t*)aligned_alloc(CACHELINE, L2_THRASH);

    if (!target || !thrash_l1 || !thrash_l2) {
        printf("alloc failed\n");
        return 1;
    }

    memset((void*)thrash_l1, 1, L1_THRASH);
    memset((void*)thrash_l2, 2, L2_THRASH);
    ((volatile uint8_t*)target)[0] = 7;

    // 1) Cold access -> should be MEM (or at least not L1)
    touch_byte(target);
    print_level("cold target load (expect MEM)");

    // 2) Immediate re-access -> should be L1 hit
    touch_byte(target);
    print_level("immediate target reload (expect L1)");

    // 3) Evict from L1 but keep in L2 -> target reload should be L2 hit
    thrash(thrash_l1, L1_THRASH);
    touch_byte(target);
    print_level("after L1 thrash, target reload (expect L2)");

    // 4) Evict from L2 too -> target reload should be MEM again
    thrash(thrash_l2, L2_THRASH);
    touch_byte(target);
    print_level("after L2 thrash, target reload (expect MEM)");

    free((void*)target);
    free((void*)thrash_l1);
    free((void*)thrash_l2);
    return 0;
}
