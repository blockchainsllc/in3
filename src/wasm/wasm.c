#include "../core/client/client.h"
#include <emscripten.h>

extern "C" {

    in3 * EMSCRIPTEN_KEEPALIVE in3_create() {
        return in3_new();
    }

    /* sends a request and stores the result in the provided buffer */
    int EMSCRIPTEN_KEEPALIVE in3_send(in3* c,char* req, char* result, int buf_size) {
        return in3_client_send(c,req,result,buf_size);
    }

    /* frees the references of the client */
    void EMSCRIPTEN_KEEPALIVE in3_dispose(in3 *a) {
        in3_free(a);
    }


}