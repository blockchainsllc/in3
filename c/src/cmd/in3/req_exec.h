#include "helper.h"

typedef struct req_exec {
  uint32_t port;
  char*    allowed_methods;
} req_exec_t;

req_exec_t* get_req_exec();
void        check_server(in3_t* c);