// @PUBLIC_HEADER
/** @file 
 * transport-handler using libcurl.
 */

#ifndef in3_curl_h__
#define in3_curl_h__

#include "../../core/client/client.h"

/**
 * the transport function using curl.
 */
in3_ret_t send_curl(char** urls, int urls_len, char* payload, in3_response_t* result);

/**
 * registers curl as a default transport.
 */
void in3_register_curl();

#endif // in3_curl_h__
