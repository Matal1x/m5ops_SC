#include <gem5/m5ops.h>
#include <stdio.h>

#define GEM5

int main(void) {
#ifdef GEM5
    uint64_t testsum = m5_sum(1, 1, 1, 1, 1, 1);
    printf("This is BUT A TEEEEEEEEEEEEST and the sum should be 6, BUT IS IT????? We check: %d\n", testsum);
#endif
}