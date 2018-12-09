#include <stdlib.h>  
#include <string.h>
#include <stdio.h>
#include <stdint.h>  
#include "client.h"  
#include "../jsmn/jsmnutil.h"
#include "../util/utils.h"
#include <errno.h> 




static int in3_client_send_intern( in3* c, json_object_t* requests, int request_count, in3_response_t* responses) {
  int i,l;
  json_object_t token;
  for (i=0;i<request_count;i++) {
    responses[i].result=malloc(200);
    if (json_get_token(requests+i,"method",&token)) 
      json_object_to_string(&token,responses[i].result);
    else
       memcpy(responses[i].result,"none",5);
  }
//   printf("Value of errno: %d\n", errno); 

  return 0;
}


int in3_client_send( in3* c, char* req, char* result, int buf_size) {
  int tokc, res=0, len,p,i;
	jsmntok_t *tokv=NULL;
  in3_response_t* responses = NULL;


   // parse the incomming request
	res = jsmnutil_parse_json(req, &tokv, &tokc);
  if (res<0 || tokc==0) 
    return IN3_ERR_INVALID_JSON;
  JSON_OBJECT(data,req, tokv) 

  if (data.tok->type==JSMN_ARRAY) {
      // request is a array, we also create a response array
      json_object_t* requests = malloc(sizeof(json_object_t)* data.tok->size);
      responses = malloc(sizeof(in3_response_t)* data.tok->size);
      json_object_to_array(&data,requests);
      res = in3_client_send_intern(c,requests, data.tok->size, responses);
      // create the results if it was succesful
      if (res>0) {
        result[0]='[';
        for (p=1,i=0,len=0;i<data.tok->size;i++) {
          if (i>0) result[p++]=',';
          len = strlen(responses[i].result);
          if (p+len>buf_size) {
            res = IN3_ERR_BUFFER_TOO_SMALL;
            break;
          }
          memcpy(result+p, responses[i].result, len);
          p+=len;
        }
        result[p++]=']';
        result[p]=0;
      }

      // clean up
      for (i=0;i<data.tok->size;i++)
        free(responses[i].result);
      free(responses);
      free(requests);
    }
    else if (data.tok->type==JSMN_OBJECT) {
      in3_response_t r;
      res = in3_client_send_intern(c,&data, 1, &r);
      if (res==0) {
        len = strlen(r.result);
        memcpy(result, r.result, len+1);
      }
      free(r.result);
    }
    else
      res = 0;


	if (tokv)
       free(tokv);

  return res;

}



int get_node_list(in3* c, u_int64_t chain_id, bool update,  in3_node_t** nodeList, u_int32_t* nodeListLength ) {
  int i;
  in3_chain_t* chain;
  for (i=0;i<c->serversCount;i++) {
    chain = c->servers + i;
    if (chain->chainId == chain_id) {
      if (chain->needsUpdate || update) {
        chain->needsUpdate=false;
        // now update the nodeList

/*

        
      // create a random seed which ensures the deterministic nature of even a partly list.
      const seed = '0x' + keccak256('0x' + Math.round(Math.random() * Number.MAX_SAFE_INTEGER).toString(16)).toString('hex')

      const nlResponse = await this.sendRPC(
        'in3_nodeList',
        [this.defConfig.nodeLimit || 0, seed, servers.initAddresses || []],
        chain, conf)
      const nl = nlResponse.result as ServerList

*/

      }
      *nodeListLength = chain->nodeListLength;
      *nodeList = chain->nodeList;
      return 0;
    }
  }

  return IN3_ERR_CHAIN_NOT_FOUND;
}
