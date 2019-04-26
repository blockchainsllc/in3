#ifndef ETH_FILTER_H
#define ETH_FILTER_H

#include "../core/util/bytes.h"
#include <stdint.h>

typedef struct in3_filter_opt_t_ {
  uint64_t  from_block;
  uint64_t  to_block;
  address_t address;
  uint32_t* topics;
  size_t    topic_count;
  void (*release)(struct in3_filter_opt_t_* fopt);
} in3_filter_opt_t;

typedef enum {
  FILTER_EVENT,
  FILTER_BLOCK,
  FILTER_PENDING,
} in3_filter_type_t;

typedef struct in3_filter_t_ {
  in3_filter_type_t type;
  in3_filter_opt_t* options;
  uint64_t          last_block;
  void (*release)(struct in3_filter_t_* f);
} in3_filter_t;

typedef struct in3_filter_handler_t_ {
  in3_filter_t** array;
  size_t         count;
  void (*release)(struct in3_filter_handler_t_* fh);
} in3_filter_handler_t;

in3_filter_opt_t*     new_filter_opt();
in3_filter_t*         new_filter(in3_filter_type_t type);
in3_filter_handler_t* new_filter_handle();

#endif //ETH_FILTER_H
