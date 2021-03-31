#include "helper.h"

typedef enum weight_enum {
  weight_test_request = 1,
  weight_health       = 2
} weight_enum_t;

uint32_t* get_weightsdata();
bool      exec_weights(in3_t* c);