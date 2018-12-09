// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "util/debug.h"
#include "util/utils.h"

int main (int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stdout,"Usage: %s hex\n",argv[0]);
    return 1;
  }

  unsigned int chars =  strlen(argv[1]);
  bytes_t* bytes     = hex2byte_bytes(argv[1],chars);
  bytes_t* shaBytes  = sha3(bytes);
  char* hex          = malloc(65);
  int8_to_char(shaBytes->data, shaBytes->len,hex);
  fprintf(stdout,"The hash of %s is %s\n",
          argv[1], hex);

  free(hex);
  b_free(bytes);
  b_free(shaBytes);

  return 0;
}