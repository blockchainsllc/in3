// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/util/debug.h"
#include "core/util/utils.h"
#include "core/client/client.h"


in3_response_t* send(char** urls,char* payload,int nodes_count) {
  printf("payload: %s\n",payload);
  in3_response_t* r = calloc(nodes_count, sizeof(in3_response_t));
  int i;
  for (i=0;i<nodes_count;i++) {
    printf("  url: %s\n",urls[i]);
    r[i].result=malloc(200);
    memcpy(r[i].result,"[{\"result\":\"0x1234\"}]",50);
  }

  return r;

}

int main (int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stdout,"Usage: %s hex\n",argv[0]);
    return 1;
  }

  in3* c = in3_new();
  c->transport = send;
  c->requestCount = 2;
  char result[500];
  in3_client_send(c, "{\"method\":\"test\",\"method2\":\"test2\"}", result,500);
  in3_free(c);

  fprintf(stdout, "result: %s ",result);

  

  

  unsigned int chars =  strlen(argv[1]);
  bytes_t* bytes     = hex2byte_new_bytes(argv[1],chars);
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