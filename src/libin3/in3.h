#ifndef in3_h__
#define in3_h__
#include <client/client.h>

extern in3 *in3_create();

/* sends a request and stores the result in the provided buffer */
extern int in3_send(in3* c,char* req, char* result, int buf_size);

/* frees the references of the client */
extern void in3_dispose(in3 *a);

 
#endif  // in3_h__