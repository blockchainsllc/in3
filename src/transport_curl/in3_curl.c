#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "in3_curl.h"
#include "../core/client/client.h"
#include "../core/util/stringbuilder.h"
#include <curl/curl.h>

/*
struct MemoryStruct {
  char *memory;
  size_t size;
};
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  in3_response_t* r = (in3_response_t*) userp;
  sb_add_range(&r->result, contents, 0, size * nmemb );
  return size * nmemb ;
}
 

void readData( char* url, char* payload, in3_response_t* r  ) {
  CURL *curl;
  CURLcode res;
  
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
 
    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)r);


    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK) {
      sb_add_chars(&r->error, "curl_easy_perform() failed:");
      sb_add_chars(&r->error, (char*) curl_easy_strerror(res));
    }
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  else {
    sb_add_chars(&r->error, "no curl:");
    sb_add_chars(&r->error, (char*)curl_easy_strerror(res));
  }

}




int send_curl(char** urls,int urls_len, char* payload, in3_response_t* result) {
 // printf("payload: %s\n",payload);
  int i;
  for (i=0;i<urls_len;i++) {
//    printf("  url: %s\n",urls[i]);
    readData(urls[i],payload, result+i );
  }
  return 0;

}
