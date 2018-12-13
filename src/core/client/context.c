#include <stdio.h>  
#include <stdint.h>  
#include <stdlib.h>  
#include <string.h>  
#include "context.h"
#include "client.h"
#include "../jsmn/jsmnutil.h"
#include "../util/stringbuilder.h"

static int get_token_size(jsmntok_t* t) {
	int i,j=1;
	switch (t->type) {
		case JSMN_PRIMITIVE:
		case JSMN_STRING:
		  return 1;
		case JSMN_OBJECT:
		  for (i=0;i<t->size;i++) 
			  j+=1+get_token_size(t+j+1);
		  return j;
		case JSMN_ARRAY:
		  for (i=0;i<t->size;i++) 
			  j+=get_token_size(t+j);
		  return j;
		default:
		  return 1;
	}
}


in3_ctx_t* new_ctx(char* req_data) {
    in3_ctx_t* c = calloc(1,sizeof(in3_ctx_t));
    c->request_data = req_data;
    c->attempt = 0;

    int tokc, res, i;
	jsmntok_t *t=NULL;

    // parse
	res = jsmnutil_parse_json(req_data, &c->tok_req, &tokc);
    if (res<0 || tokc==0) {
        ctx_set_error(c,"Error parsing the JSON-request!",0);
        return c;
    }

    if (c->tok_req[0].type==JSMN_OBJECT) {
        // it is a single result
        c->requests      = malloc(sizeof(jsmntok_t*));
        c->requests[0]   = c->tok_req;
        c->len          = 1;
    }
    else if (c->tok_req[0].type==JSMN_ARRAY) {
        c->len        = c->tok_req[0].size;
        c->requests   = malloc(sizeof(jsmntok_t*)*c->len);
        t = c->tok_req +1;
        for (i=0;i<c->len;i++) {
            c->requests[i] = t;
            t+=get_token_size(t);
        }
    }
    else 
        ctx_set_error(c,"The Request is not a valid structure!",0);

    if (c->len) 
       c->requests_configs = calloc(c->len, sizeof(in3_request_config_t));

    return c;
}



int ctx_parse_response(in3_ctx_t* ctx, char* response_data) {
    int tokc, res, i;
	jsmntok_t *t=NULL;

    // assign data
    ctx->response_data = response_data;

    // parse
	res = jsmnutil_parse_json(response_data, &ctx->tok_res, &tokc);
    if (res<0 || tokc==0) 
      return ctx_set_error(ctx,"Error parsing the JSON-respomse!",IN3_ERR_INVALID_JSON);

    if (ctx->tok_res[0].type==JSMN_OBJECT) {
        // it is a single result
        ctx->responses    = malloc(sizeof(jsmntok_t*));
        ctx->responses[0] = ctx->tok_res;
        ctx->len          = 1;
    }
    else if (ctx->tok_res[0].type==JSMN_ARRAY) {
        ctx->len        = ctx->tok_res[0].size;
        ctx->responses  = malloc(sizeof(jsmntok_t*)*ctx->len);
        t = ctx->tok_res +1;
        for (i=0;i<ctx->len;i++) {
            ctx->responses[i] = t;
            t+=get_token_size(t);
        }
    }
    else
      return ctx_set_error(ctx,"The response must be a Object or Array",IN3_ERR_INVALID_JSON);
    
    return 0;
}

void free_ctx(in3_ctx_t* ctx) {
    int i;
    if (ctx->error)         free(ctx->error);
    free_ctx_nodes(ctx->nodes);
    if (ctx->response_data) free(ctx->response_data);
    if (ctx->requests)      free(ctx->requests);
    if (ctx->responses)     free(ctx->responses);
    if (ctx->tok_res)       free(ctx->tok_res);
    if (ctx->tok_req)       free(ctx->tok_req);
    if (ctx->requests_configs ) {
        for (i=0;i<ctx->len;i++) {
            if (ctx->requests_configs[i].signaturesCount) 
               free(ctx->requests_configs[i].signatures);
        }
        free(ctx->requests_configs);
    }

    free(ctx);
}

jsmntok_t* ctx_get_token(char* str, jsmntok_t* root, char* key) {
   int i,n,l=strlen(key);
   jsmntok_t* c = root+1;
 
   for (i=0;i<root->size;i++) {
	 n = c->end - c->start;
	 // the key must be a string
	 if (c->type != JSMN_STRING)
	    return NULL;
	 // if the key matches we retrun the next token 
	 if (l==n && !strncmp(str + c->start, key, n)) 
       return c+1;

	 // if not we have to check the value
	 c += get_token_size(c+1)+1;
   }
   return NULL;
}

bool ctx_equals(char* str, jsmntok_t* c, char* val) {
    if (!c) return false;
	int  n = c->end - c->start;
    return strlen(val)==n && !strncmp(str + c->start, val, n);
}

uint64_t ctx_to_long(char* str, jsmntok_t* c, uint64_t defVal) {
    if (!c) return defVal;
	int  n = c->end - c->start;
    char *idval = malloc(n+1);
    idval[n] = 0;
	strncpy(idval, str + c->start, n);
	uint64_t val = atol(idval);
	free(idval);
	return val;
}

bytes_t* ctx_to_bytes(char* str, jsmntok_t* c, int min_len) {
    if (!c) return NULL;
	int  s= c->start, n = c->end - s;
    // skip tzhe first 2 characters if it starts with '0x'
    if (n >2 && str[s+1]=='x' ) {
        s+=2;
        n-=2;
    }
    int bytes_len = (n & 1) ? (n + 1) / 2 : n/2;
    int prefix    = min_len>bytes_len ? min_len-bytes_len : 0;
    uint8_t *b    = calloc(1,bytes_len+prefix);
	if (hex2byte_arr( str+ s ,n,b+prefix,bytes_len)<0) {
      free(b);
      return NULL;
    }
	return b_new((char*)b,bytes_len+prefix);
}

int ctx_cpy_string(char* str, jsmntok_t* c, char* dst) {
    if (!c) return 0;
    int n=c->end-c->start;
    memcpy(dst,str + c->start, n);
    return n;
}

static unsigned long counter = 1;


int ctx_create_payload(in3_ctx_t* c, sb_t* sb) {

    int i,lv=0;
    jsmntok_t* r,*t;
    char temp[100];
    sb_add_char(sb,'[');

    for (i=0;i<c->len;i++) {
        if (i>0) sb_add_char(sb,',');
        sb_add_char(sb,'{');
        r = c->requests[i];
        if ((t=ctx_get_token(c->request_data,r,"id"))==NULL) 
            sb_add_key_value(sb, "id", temp,  sprintf(temp,"%lu",counter++), false);
        else
            sb_add_key_value(sb,"id", c->request_data + t->start, t->end-t->start, t->type==JSMN_STRING);
        sb_add_char(sb,',');
        sb_add_key_value(sb,"jsonrpc", "2.0",3,true);
        sb_add_char(sb,',');
        if ((t=ctx_get_token(c->request_data,r,"method"))==NULL) 
            return ctx_set_error(c,"missing method-property in request",IN3_ERR_REQUEST_INVALID);
        else 
            sb_add_key_value(sb,"method", c->request_data + t->start, t->end-t->start, true);
        sb_add_char(sb,',');
        if ((t=ctx_get_token(c->request_data,r,"params"))==NULL) 
            sb_add_key_value(sb,"params", "[]",2,false);
        else 
            sb_add_key_value(sb,"params", c->request_data + t->start, t->end-t->start, false);
        sb_add_char(sb,',');

        // add in3
        in3_request_config_t* rc = c->requests_configs+i;
        sb_add_range(sb,temp,0, sprintf(temp,"\"in3\":{\"chainId\":\"0x%llx\"", rc->chainId));
        if (rc->clientSignature) 
           sb_add_bytes(sb,",\"clientSignature\":",rc->clientSignature,1,false );
        if (rc->finality) 
           sb_add_range(sb,temp,0,sprintf(temp,",\"finality\":%i", rc->finality));
        if (rc->includeCode)
           sb_add_chars(sb,",\"includeCode\":true");
        if (rc->latestBlock)
           sb_add_range(sb,temp,0,sprintf(temp,",\"latestBlock\":%i", rc->latestBlock));
        if (rc->signaturesCount) 
           sb_add_bytes(sb,",\"signatures\":",rc->signatures,rc->signaturesCount,true);
        if (rc->useFullProof)
           sb_add_chars(sb,",\"useFullProof\":true");
        if (rc->verification==VERIFICATION_PROOF)
           sb_add_chars(sb,",\"verification\":\"proof\"");
        else if (rc->verification==VERIFICATION_PROOF_WITH_SIGNATURE)
           sb_add_chars(sb,",\"verification\":\"proofWithSignature\"");
        if (rc->verifiedHashesCount) 
           sb_add_bytes(sb,",\"verifiedHashes\":",rc->verifiedHashes,rc->verifiedHashesCount,true);

        sb_add_range(sb,"}}",0,2);
    }
    sb_add_char(sb,']');
    return 0;
}


int ctx_set_error(in3_ctx_t* c, char* msg, int errnumber) {
    int l = strlen(msg);
	char* dst;
    if (c->error) {
        dst= malloc(l+2+strlen(c->error));
    	strcpy(dst,msg);
        dst[l]='\n';
    	strcpy(dst+l+1,c->error);
        free(c->error);
    }
    else  {
        dst= malloc(l+1);
    	strcpy(dst,msg);
    }
    c->error = dst;
    return errnumber;
}


int ctx_nodes_len (node_weight_t* c) {
    int all=0;
    while (c) {
        all++;
        c=c->next;
    }
    return all;
}
void free_ctx_nodes (node_weight_t* c) {
    node_weight_t* p;
    while (c) {
        p = c;
        c=c->next;
        free(p);
    }
}
