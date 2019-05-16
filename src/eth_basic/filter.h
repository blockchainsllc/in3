#ifndef FILTER_H
#define FILTER_H

#include <client/client.h>
#include <client/context.h>

in3_filter_opt_t* filter_opt_new(); /**< creates and returns a in3_filter_opt_t object */
in3_filter_t*     filter_new(in3_filter_type_t ft);
size_t            filter_add(in3_t* in3, in3_filter_type_t type, in3_filter_opt_t* options);
bool              filter_remove(in3_t* in3, size_t id);
int               filter_get_changes(in3_ctx_t* ctx, size_t id, void (*parse_result)(in3_filter_type_t type, void* result, size_t len, void* userdata), void* userdata);
int               filter_opt_from_json(in3_filter_opt_t** fopt, d_token_t* tx_params);
sb_t*             filter_opt_to_json_str(in3_filter_opt_t* fopt, sb_t* sb);

#endif //FILTER_H
