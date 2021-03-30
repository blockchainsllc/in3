#include "helper.h"

typedef enum output {
  weight_test_request = 1,
  weight_health       = 2
} output_t;

uint32_t* get_weightsdata();