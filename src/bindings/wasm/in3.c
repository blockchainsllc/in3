#include "in3.h"
#include <client/client.h>
#include <in3_curl.h>

in3_t* in3_create() {
    in3_t* i = in3_new();
    i->transport = send_curl;
    return i;
}
/* sends a request and stores the result in the provided buffer */
int in3_send(in3_t* c, char* method, char* params ,char** result,  char** error) {
  return in3_client_rpc(c,method,params ,result, error);
}


/* frees the references of the client */
void in3_dispose(in3_t* a) {
    in3_free(a);
}

