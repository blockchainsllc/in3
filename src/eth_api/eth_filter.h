#ifndef ETH_FILTER_H
#define ETH_FILTER_H

#include "eth_api.h"
#include <client/client.h>

in3_filter_opt_t* new_filter_opt();                                                                        /**< creates and returns a in3_filter_opt_t object */
size_t            eth_newFilter(in3_t* in3, in3_filter_opt_t* options);                                    /**< creates a new event filter with specified options and returns its id (>0) on success or 0 on failure */
size_t            eth_newBlockFilter(in3_t* in3);                                                          /**< creates a new block filter with specified options and returns its id (>0) on success or 0 on failure */
size_t            eth_newPendingTransactionFilter(in3_t* in3);                                             /**< creates a new pending txn filter with specified options and returns its id on success or 0 on failure */
bool              eth_uninstallFilter(in3_t* in3, size_t id);                                              /**< uninstalls a filter and returns true on success or false on failure */
int               eth_getFilterChanges(in3_t* in3, size_t id, bytes32_t** block_hashes, eth_log_t** logs); /**< sets the logs (for event filter) or blockhashes (for block filter) that match a filter; returns <0 on error, otherwise no. of block hashes matched (for block filter) or 0 (for log filer) */

#endif //ETH_FILTER_H
