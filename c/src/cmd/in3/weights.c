#include "weights.h"
#include "helper.h"

uint32_t* get_weightsdata() {
  static uint32_t weightdata = 0;
  return &weightdata;
}