#ifndef ETH_FILTER_H
#define ETH_FILTER_H

#include "eth_api.h"
#include <client/client.h>

in3_filter_opt_t* new_filter_opt();
in3_filter_t*     new_filter(in3_filter_type_t type);

size_t eth_newFilter(in3_t* in3, in3_filter_opt_t* options);
size_t eth_newBlockFilter(in3_t* in3, in3_filter_opt_t* options);
size_t eth_newPendingTransactionFilter(in3_t* in3, in3_filter_opt_t* options);
bool   eth_uninstallFilter(in3_t* in3, size_t id);
int    eth_getFilterChanges(in3_t* in3, size_t id, bytes32_t** block_hashes, eth_log_t** logs);

#endif //ETH_FILTER_H
