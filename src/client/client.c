#include <stdlib.h>  
#include <stdint.h>  
#include "client.h"  
#include "../jsmn/jsmnutil.h"
#include "../util/utils.h"





static int in3_client_send_intern( in3* c, json_object_t* requests, int request_count) {

}


int in3_client_send( in3* c, char* req, char* result, int buf_size) {
    int tokc, res=1;
	jsmntok_t *tokv;


    // parse the incomming request
	jsmnutil_parse_json(req, &tokv, &tokc);
    JSON_OBJECT(data,req, tokv) 

    if (data.tok->type==JSMN_ARRAY) {
      json_object_t* requests = malloc(sizeof(json_object_t)* data.tok->size);
      json_object_to_array(&data,requests);
      res = in3_client_send_intern(c,requests, data.tok->size);
      free(requests);
    }
    else if (data.tok->type==JSMN_OBJECT) 
      res = in3_client_send_intern(c,&data, 1);
    else
      res = 0;


	if (tokv)
       free(tokv);

    return res;

}

