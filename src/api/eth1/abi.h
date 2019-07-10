#include "../core/util/bytes.h"
#include "../core/util/data.h"

#if !defined(_ETH_API__ABI_H_)
#define _ETH_API__ABI_H_

typedef enum {
  A_UINT    = 1,
  A_INT     = 2,
  A_BYTES   = 3,
  A_BOOL    = 4,
  A_ADDRESS = 5,
  A_TUPLE   = 6,
  A_STRING  = 7
} atype_t;

typedef struct el {
  atype_t type;
  bytes_t data;
  uint8_t type_len;
  int     array_len;
} var_t;

typedef struct {
  var_t*           in_data;
  var_t*           out_data;
  bytes_builder_t* call_data;
  var_t*           current;
  char*            error;
  int              data_offset;
} call_request_t;

call_request_t* parseSignature(char* sig);
json_ctx_t*     req_parse_result(call_request_t* req, bytes_t data);
void            req_free(call_request_t* req);
int             set_data(call_request_t* req, d_token_t* data, var_t* tuple);
var_t*          t_next(var_t* t);
int             word_size(int b);

#endif // _ETH_API__ABI_H_
