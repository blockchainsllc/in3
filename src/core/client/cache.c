#include "../util/utils.h"
#include <stdlib.h>
#include "cache.h"
#include "context.h"
#include "stdio.h"
#include "../jsmn/jsmnutil.h"

int in3_cache_init(in3* c) {
    int i;
    for (i=0;i<c->serversCount;i++) in3_cache_update_nodelist(c,c->servers+i);
    return 0;
}

int in3_cache_update_nodelist(in3* c, in3_chain_t* chain) {
    if (!c->cacheStorage) return 0;
    char key[200];
    sprintf(key,"nodelist_%llx",chain->chainId);

    bytes_t* b = c->cacheStorage->get_item(key);
    if (b) {
        in3_ctx_t ctx;
        ctx.error = NULL;
        ctx.tok_res=NULL;
        int tokc;
        ctx.response_data =(char*) b->data;
        if (jsmnutil_parse_json_range((char*)b->data,b->len, &ctx.tok_req, &tokc)>=0 && in3_client_fill_chain(chain, &ctx,ctx.tok_req)>=0)
           chain->needsUpdate=false;
        if (ctx.error) free(ctx.error);
        if (ctx.tok_req) free(ctx.tok_req);
        b_free(b);
    }
    return 0;
}
int in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain) {
    jsmntok_t* r = ctx_get_token(ctx->response_data, ctx->responses[0],"result");
    if (r) {
        char key[200];
        sprintf(key,"nodelist_%llx",chain->chainId);
        bytes_t b;
        b.len = r->end - r->start;
        b.data = (uint8_t*)(ctx->response_data + r->start);
        ctx->client ->cacheStorage->set_item(key,&b);
    }
    return 0;
}
