// @PUBLIC_HEADER
/** @file 
 * transport-handler using libcurl.
 */

#ifndef in3_curl_h__
#define in3_curl_h__

#include "client.h"

in3_ret_t send_curl(char** urls, int urls_len, char* payload, in3_response_t* result);

#endif // in3_curl_h__
