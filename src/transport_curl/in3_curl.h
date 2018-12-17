
/** @file 
 * transport-handler using libcurl.
 */ 

#ifndef in3_curl_h__
#define in3_curl_h__

#include "../core/client/client.h"

int send_curl(char** urls,int urls_len, char* payload, in3_response_t* result) ;
 
#endif  // in3_curl_h__