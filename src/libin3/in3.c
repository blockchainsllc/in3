#include "in3.h"
#include <client/client.h>

in3 *in3_create() {
    return in3_new();
}

/* sends a request and stores the result in the provided buffer */
int in3_send(in3* c,char* req, char* result, int buf_size) {
    return in3_client_send(c,req,result,buf_size);
}

/* frees the references of the client */
void in3_dispose(in3 *a) {
    in3_free(a);
}

