#ifndef FILTER_H
#define FILTER_H

#include <client/client.h>

in3_filter_opt_t* filter_new_opt(); /**< creates and returns a in3_filter_opt_t object */
in3_filter_t*     filter_new(in3_filter_type_t ft);
size_t            filter_add(in3_t* in3, in3_filter_type_t type, in3_filter_opt_t* options);
bool              filter_remove(in3_t* in3, size_t id);
bool              filter_valid_addrs(d_token_t* addr);
bool              filter_valid_topics(d_token_t* topics);

#endif //FILTER_H
