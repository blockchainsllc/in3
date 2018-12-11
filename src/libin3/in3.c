#include "in3.h"
#include <client/client.h>
#include <in3_curl.h>


in3 *in3_create() {
    in3* i = in3_new();
    i->transport = send_curl;
    return i;
}

/* sends a request and stores the result in the provided buffer */
int in3_send(in3* c,char* req, char* result, int buf_size) {
    return in3_client_send(c,req,result,buf_size);
}

/* frees the references of the client */
void in3_dispose(in3 *a) {
    in3_free(a);
}

