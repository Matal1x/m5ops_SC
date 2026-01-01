#ifndef ADDRESS_SET_ADAPTER_H
#define ADDRESS_SET_ADAPTER_H


#include "threshold_group_testing.h"  

eviction_test_func_t create_eviction_tester(void);


address_set_t generate_candidate_set(void *target_addr,
                                     size_t num_candidates,
                                     const cache_config_t *cfg);
                                     
#endif