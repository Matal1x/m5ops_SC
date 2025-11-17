#include <stdio.h>
#include <gem5/m5ops.h>

int main()
{
    printf("Requesting last hit level…\n");

    uint64_t level = m5_get_last_hit_level();
    printf("Last hit level = %lu\n", level);

    return 0;
}
