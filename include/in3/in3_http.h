// @PUBLIC_HEADER
/** @file 
 * transport-handler using simple http.
 */

#ifndef in3_http_h__
#define in3_http_h__

#include "client.h"

in3_ret_t send_http(char** urls, int urls_len, char* payload, in3_response_t* result);

#endif // in3_http_h__
