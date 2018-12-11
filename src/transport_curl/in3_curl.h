#ifndef in3_curl_h__
#define in3_curl_h__

#include "../core/client/client.h"

in3_response_t* send_curl(char** urls,char* payload,int nodes_count) ;
 
#endif  // in3_curl_h__