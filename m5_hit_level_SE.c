#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gem5/m5ops.h>

#define CACHELINE 64
#define PAGE      4096

static inline uint8_t load8(volatile uint8_t *p) { return *p; }

static void print_level(const char *tag)
{
    uint64_t level = m5_get_last_hit_level();
    printf("%s: last_hit_level=%lu\n", tag, (unsigned long)level);
}

int main(void)
{
    // Allocate 9 pages so we can touch 9 addresses with same L1 set (8-way -> eviction).
    size_t region_size = 9 * PAGE;
    volatile uint8_t *region = (volatile uint8_t*)aligned_alloc(PAGE, region_size);
    if (!region) return 1;
    memset((void*)region, 0, region_size);

    // Choose target at offset 0. Do NOT store to it before the "cold" load.
    volatile uint8_t *target = region + 0;

    // 1) Cold load (should be MEM on first touch, or at least not guaranteed L1)
    (void)load8(target);
    print_level("cold target load (expect MEM or not-L1)");

    // 2) Immediate reload (should be L1 hit)
    (void)load8(target);
    print_level("immediate reload (expect L1)");

    // 3) Evict target from L1 by filling same set with 8+ other lines (same set via +4KiB stride)
    for (int i = 1; i <= 8; i++) {
        (void)load8(region + i * PAGE);
    }

    // Reload target: should miss in L1; likely hit in L2 if L2 is inclusive/shared and big enough
    (void)load8(target);
    print_level("after L1 same-set eviction (expect L2)");

    // 4) Thrash L2 heavily to force target to memory (simple large walk)
    // Adjust size if needed; must exceed L2 capacity meaningfully.
    size_t l2_thrash = 8 * 1024 * 1024;
    volatile uint8_t *big = (volatile uint8_t*)aligned_alloc(PAGE, l2_thrash);
    if (!big) return 1;
    memset((void*)big, 1, l2_thrash);

    for (size_t off = 0; off < l2_thrash; off += CACHELINE) {
        (void)load8(big + off);
    }

    (void)load8(target);
    print_level("after L2 thrash (expect MEM)");

    free((void*)region);
    free((void*)big);
    return 0;
}
