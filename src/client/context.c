#include <stdio.h>  
#include <stdint.h>  
#include <stdlib.h>  
#include <string.h>  
#include "context.h"
#include "client.h"
#include "../jsmn/jsmnutil.h"

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

u_int64_t ctx_to_long(char* str, jsmntok_t* c, u_int64_t defVal) {
    if (!c) return defVal;
	int  n = c->end - c->start;
    char *idval = malloc(n+1);
    idval[n] = 0;
	strncpy(idval, str + c->start, n);
	u_int64_t val = atol(idval);
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
static int add_key_value(char* b,char* key, char* value, int lv, bool as_string) {
    if (lv==0) return 0;
    int p=0, lk=strlen(key);
    b[p++]='"';
    memcpy(b+p,key,lk);
    p+=lk;
    b[p++]='"';
    b[p++]=':';
    if (as_string)  b[p++]='"';
    memcpy(b+p,value,lv);
    p+=lv;
    if (as_string)  b[p++]='"';
    return p;
}

static int write_bytes(char* b, char* prefix, bytes_t* bytes, int len) {
    int i,p=strlen(prefix);
    memcpy(b,prefix,p);
    b[p++]='[';
    for (i=0;i<len;i++)  {
        if (i>0) b[p++]=',';
        b[p++]='"';
        b[p++]='0';
        b[p++]='x';
        int8_to_char(bytes[i].data,bytes[i].len,b+p);
        p+=bytes[i].len;
        b[p++]='"';
    }
    b[p++]=']';
    return p;
}
int ctx_create_payload(in3_ctx_t* c, char* b, int buffer_size) {
    int p=1,i,lv=0;
    jsmntok_t* r,*t;
    char temp[100];
    b[0]='[';

    for (i=0;i<c->len;i++) {
        if (i>0) b[p++]=',';
        b[p++]='{';
        r = c->requests[i];
        if ((t=ctx_get_token(c->request_data,r,"id"))==NULL) 
            p+=add_key_value(b+p,"id", temp,  sprintf(temp,"%lu",counter++), false);
        else
            p+=add_key_value(b+p,"id", c->request_data + t->start, t->end-t->start, t->type==JSMN_STRING);
        b[p++]=',';
        p+=add_key_value(b+p,"jsonrpc", "2.0",3,true);
        b[p++]=',';
        if ((t=ctx_get_token(c->request_data,r,"method"))==NULL) 
            return ctx_set_error(c,"missing method-property in request",IN3_ERR_REQUEST_INVALID);
        else 
            p+=add_key_value(b+p,"method", c->request_data + t->start, t->end-t->start, true);

        b[p++]=',';
        if ((t=ctx_get_token(c->request_data,r,"params"))==NULL) 
            p+=add_key_value(b+p,"params", "[]",2,false);
        else 
            p+=add_key_value(b+p,"params", c->request_data + t->start, t->end-t->start, false);
        b[p++]=',';

        // add in3
        in3_request_config_t* rc = c->requests_configs+i;
        p+=sprintf(b+p,"\"in3\":{\"chainId\":\"0x%llx\"", rc->chainId);
        if (rc->clientSignature) {
            p+=sprintf(b+p,",\"clientSignature\":\"0x");
            int8_to_char(rc->clientSignature->data,rc->clientSignature->len,b+p);
            p+=rc->clientSignature->len*2;
            b[p++]='"';
        }
        if (rc->finality) 
           p+=sprintf(b+p,",\"finality\":%i", rc->finality);
        if (rc->includeCode)
           p+=sprintf(b+p,",\"includeCode\":true");
        if (rc->latestBlock)
           p+=sprintf(b+p,",\"latestBlock\":%i", rc->latestBlock);
        if (rc->signaturesCount) 
           p+=write_bytes(b+p,",\"signatures\":",rc->signatures,rc->signaturesCount);
        if (rc->useFullProof)
           p+=sprintf(b+p,",\"useFullProof\":true");
        if (rc->verification==VERIFICATION_PROOF)
           p+=sprintf(b+p,",\"verification\":\"proof\"");
        else if (rc->verification==VERIFICATION_PROOF_WITH_SIGNATURE)
           p+=sprintf(b+p,",\"verification\":\"proofWithSignature\"");
        if (rc->verifiedHashesCount) 
           p+=write_bytes(b+p,",\"verifiedHashes\":",rc->verifiedHashes,rc->verifiedHashesCount);
           

        b[p++]='}';
        b[p++]='}';
    }
    b[p++]=']';
    b[p++]=0;
    return p;
}


int ctx_set_error(in3_ctx_t* c, char* msg, int errnumber) {
    if (c->error) free(c->error);
    int l = strlen(msg);
	char* dst = malloc(l+1);
	strcpy(dst,msg);
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
