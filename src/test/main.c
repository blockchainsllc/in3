// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <util/debug.h>
#include <util/utils.h>
#include <client/client.h>
#include <in3_curl.h>
#include <util/mem.h>

/*
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
*/


uint64_t getChainId(char* name) {
  if (strcmp(name,"mainnet")==0)    return 0x01L;
  if (strcmp(name,"kovan")==0)      return 0x2aL;
  if (strcmp(name,"tobalaba")==0)   return 0x44dL;
  if (strcmp(name,"evan")==0)       return 0x4b1L;
  if (strcmp(name,"evan")==0)       return 0x4b1L;
  if (strcmp(name,"ipfs")==0)       return 0x7d0;
  return _atol(name);
}

int main (int argc, char *argv[])
{
  int i;
  if (argc < 2) {
    fprintf(stdout,"Usage: %s hex\n",argv[0]);
    return 1;
  }

  char* method=NULL;
  char params[5000];
  params[0]='[';
  int p=1;
  in3_t* c = in3_new();
  c->transport = send_curl;
  c->requestCount = 1;

  // fill from args
  for (i=1;i<argc;i++) {
    if (strcmp(argv[i],"--chain")==0)
       c->chainId = getChainId(argv[++i]);
    else if (strcmp(argv[i],"--proof")==0) {
      if (strcmp(argv[i+1],"none")==0) 
         c->proof = PROOF_NONE;
      else if (strcmp(argv[i+1],"standard")==0) 
         c->proof = PROOF_STANDARD;
      else if (strcmp(argv[i+1],"full")==0) 
         c->proof = PROOF_FULL;
      else {
        printf("Invalid Argument for proof: %s\n",argv[i+1]);
        return 1;
      }
      i++;
    }
    else {
      if (method==NULL)
        method=argv[i];
      else {
        if (p>1) params[p++]=',';
        if (argv[i][0]=='{' || strcmp(argv[i],"true")==0 || strcmp(argv[i],"false")==0)
          p+=sprintf(params+p,"%s",argv[i]);
        else
          p+=sprintf(params+p,"\"%s\"",argv[i]);
      }
    }
  }
  params[p++]=']';
  params[p]=0;


 
  char* result;
  char* error;;

  in3_client_rpc(c, method, params, &result, &error);
  in3_free(c);

  if (error) {
    fprintf(stderr, "Error: %s\n",error);
    free(error);
    return 1;
  }
  else {
     printf( "%s\n",result);
     free(result);
  }
  return 0;
}