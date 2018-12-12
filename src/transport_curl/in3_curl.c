#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "in3_curl.h"
#include "../core/client/client.h"
#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    mem->memory = NULL;
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}
 

void readData( char* url, char* payload, in3_response_t* r  ) {
  CURL *curl;
  CURLcode res;
  struct MemoryStruct chunk;
  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 

  
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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);


    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK) {
      r->error=malloc(100);
      sprintf(r->error, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else {
//      printf("%lu bytes retrieved\n%s", (unsigned long)chunk.size, chunk.memory);
      r->result = chunk.memory;
      chunk.memory=NULL;
    }
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  else {
    r->error=malloc(100);
    sprintf(r->error, "no curl: %s\n", curl_easy_strerror(res));
  }

}




in3_response_t* send_curl(char** urls,char* payload,int nodes_count) {
 // printf("payload: %s\n",payload);
  in3_response_t* r = calloc(nodes_count, sizeof(in3_response_t));
  int i;
  for (i=0;i<nodes_count;i++) {
//    printf("  url: %s\n",urls[i]);
    readData(urls[i],payload, r+i );
  }
  return r;

}
