#ifndef FILTER_H
#define FILTER_H

#include "../../core/client/client.h"
#include "../../core/client/context.h"

in3_ret_t filter_add(in3_t* in3, in3_filter_type_t type, char* options);
bool      filter_remove(in3_t* in3, size_t id);
in3_ret_t filter_get_changes(in3_ctx_t* ctx, size_t id, sb_t* result);
bool      filter_opt_valid(d_token_t* tx_params);

#endif //FILTER_H
