#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
void hex(bytes_t b) {
  printf("0x");
  for (int i = 0; i < b.len; i++) printf("%02x", b.data[i]);
  printf("\n");
}

int main(int argc, char** argv) {
  char*     msg = "abcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklmabcdefghijklm";
  bytes32_t p;
  zkcrypto_initialize();
  zkcrypto_pk_from_seed(bytes((void*) msg, 32), p);
  hex(bytes(p, 32));

  return 0;
}