/** @file 
 * simple commandline-util parsing json and creating bin
 * */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/data.h>
#include <util/mem.h>

/*
void newp() {
  char* js = "{ \"key\":\"value\", \"array\":[1,2,3,4,\"test\"], \"sub\":{ \"a\":3}, \"bytes\":\"0xabcdef1234567890\"  }";
  d_token_t* root;
  int tokc;

  if (parse_json(js,&root,&tokc)==0) {
    char* val = d_get_string(root,"key");
    bytes_t* bytes = d_get_bytes(root,"bytes");
    b_print(bytes);

    str_range_t s = d_to_json( d_get(root,key("sub")));
    char tmp[1000];
    strncpy(tmp,s.data,s.len);
    printf("json = | %s | ",tmp);
  }

}
*/

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

  json_parsed_t* ctx = parse_json(input);
  if (!ctx)
    printf("Invalid Json : %s\n", input);
  else {
    bytes_builder_t* bb = bb_new();
    d_serialize_binary(bb, ctx->items);

    if (strcmp(format, "hex") == 0) {
      for (i = 0; i < bb->b.len; i++) printf("%02x", bb->b.data[i]);
      printf("\n");
    } else if (strcmp(format, "cstr") == 0) {
      unsigned char c = 0, is_hex = 0;

      for (i = 0; i < bb->b.len; i++) {
        c      = bb->b.data[i];
        is_hex = c < ' ' || c > 0x7E || c == 0x5C || c == '"' || (is_hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')));
        printf(is_hex ? "\\x%02x" : "%c", c);
      }
      printf("\n len = %u\n", bb->b.len);
    } else {
      printf("unsuported output format %s!\n", format);
      return 1;
    }
  }

  //  printf("INPUT: %s", input);
  return 0;
}