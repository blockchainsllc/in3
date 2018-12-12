#include "in3.h"
#include <client/client.h>
#include <in3_curl.h>


in3 *in3_create() {
    in3* i = in3_new();
    i->transport = send_curl;
    return i;
}
/* sends a request and stores the result in the provided buffer */
int in3_send(in3* c, char* method, char* params ,char* result, int buf_size, char* error) {
  return in3_client_rpc(c,method,params ,result,buf_size, error);
}


/* frees the references of the client */
void in3_dispose(in3 *a) {
    in3_free(a);
}

