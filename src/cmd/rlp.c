/** @file 
 * simple commandline-util parsing json and creating bin
 * */

#include <math.h>
#include <rlp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/data.h>
#include <util/utils.h>

char* read_from_stdin(FILE* file) {
  if (file == NULL) {
    printf("File not found!");
    _Exit(1);
    return NULL;
  }

  size_t   allocated = 1024;
  size_t   len       = 0;
  uint8_t* buffer    = _malloc(1025);
  size_t   r;

  while (1) {
    r = fread(buffer + len, 1, allocated - len, file);
    len += r;
    if (feof(file)) break;
    buffer = _realloc(buffer, allocated * 2 + 1, allocated);
    allocated *= 2;
  }

  buffer[len] = 0;
  if (file != stdin) fclose(file);
  return (char*) buffer;
}

void write(bytes_t* data, char* l) {
  bytes_t t;
  char    prefix[100];
  int     i, j, type, p = strlen(l), d;
  for (i = 0;; i++) {
    type = rlp_decode(data, i, &t);
    if (type == 0) return;
    if (type == 1) {
      if (t.len == 0)
        d = printf("%s0", l);
      else if (t.len < 6)
        d = printf("%s%llu", l, bytes_to_long(t.data, t.len));
      else
        d = printf("%sDATA", l);
      for (j = d - p; j < 12; j++) printf(" ");
      if (t.len > 0)
        printf("0x");
      else
        printf("<EMPTY>");

      for (j = 0; j < t.len; j++)
        printf("%02x", t.data[j]);

      printf("\n");

    } else if (type == 2) {
      printf("%s[\n", l);
      sprintf(prefix, "%s   ", l);
      write(&t, prefix);
      printf("%s]\n", l);
    }
  }
}

int main(int argc, char* argv[]) {
  char* default_format = "hex";
  char* input          = NULL;
  char* format         = default_format;

  int i;

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0)
      input = read_from_stdin(fopen(argv[++i], "r"));
    else if (strcmp(argv[i], "-o") == 0)
      format = argv[++i];
    else
      input = argv[i];
  }

  if (input == NULL) input = read_from_stdin(stdin);

  if (input[0] == '0' && input[1] == 'x') input += 2;

  write(hex2byte_new_bytes(input, strlen(input)), "");

  //  printf("INPUT: %s", input);
  return 0;
}