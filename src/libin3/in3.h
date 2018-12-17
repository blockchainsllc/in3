
/** @file 
 * the entry-points for the shares library.
 * */ 
#ifndef in3_h__
#define in3_h__
#include <client/client.h>

/** creates a new client */
extern in3_t* in3_create();

/** sends a request and stores the result in the provided buffer */
extern int in3_send(in3_t* c, char* method, char* params ,char* result, int buf_size, char* error);


/** frees the references of the client */
extern void in3_dispose(in3_t *a);

 
#endif  // in3_h__