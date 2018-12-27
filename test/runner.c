#ifndef TEST 
   #define TEST
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <core/client/client.h>
#include <core/client/context.h>
#include <core/util/utils.h>
#include <eth_full/eth_full.h>


#define ERROR(s) printf("Error: %s",s)

char* readContent(char* name) {
    char temp[500];
    sprintf(temp,  strchr(name,'.')==NULL ? "../test/testdata/%s.json" : "%s", name);
    FILE *file = fopen(temp, "r");
    if (file==NULL) {
        ERROR("could not open the file");
        return NULL;
    }

    size_t allocated = 1024, len = 0, r=0;
    char *buffer = malloc(1024);
    while (1)
    {
            r = fread(buffer + len, 1, allocated - len-1, file);
            len += r;
            if (feof(file))  break;
            buffer = realloc(buffer, allocated *= 2);
    }
    buffer[len]=0;

    if (file)        
        fclose(file);

    return buffer;    
}

static char* _tmp_str;
static d_token_t* _tmp_response;

static  int send_mock(char** urls,int urls_len, char* payload, in3_response_t* result) {
    // printf("payload: %s\n",payload);
    int i;
    for (i=0;i<urls_len;i++) {
        str_range_t r = d_to_json(d_get_at(_tmp_response,i));
        sb_add_char( &result->result, '[');
        sb_add_range( &result->result, r.data ,  0, r.len);
        sb_add_char( &result->result, ']');
    }
    return 0;
}



int execRequest(in3_t *c, d_token_t* test) {
    d_token_t* request  = d_get(test,key("request"));
    d_token_t* response = d_get(test,key("response"));
    d_token_t* config   = d_get(request,key("config"));
    d_token_t* t;
    char* method;
    char params[10000];

    // configure in3
    c->requestCount = (t=d_get(config,key("requestCount"))) ?  d_int(t) : 1;
    method = d_get_string(request,"method");

    str_range_t s = d_to_json( d_get(request, key("params")) );
    if (!method) {
        printf("NO METHOD");
        return -1;
    }
    if (!s.data) {
        printf("NO PARAMS");
        return -1;
    }
    strncpy(params,s.data,s.len);
    params[s.len]=0;

    char* res, *err;
    int success =  d_get_intkd(test,key("success"),true);

    _tmp_response = response;

    in3_client_rpc(c,method,params,&res,&err);

    if (err && res) {
        printf("Error and Result set");
        _free(err);
        _free(res);
        return -1;

    }
    else if (err) {
        if (success) {
            printf("Failed: %s", err);
           _free(err);
           return -1;
        }
        /*
        if ((t=ctx_get_token(str,test,"error")) && strncmp(str+t->start, err, t->end - t->start)!=0) {
                printf("wrong error: %s", err);
               _free(err);
               return -1;
        }
        */
        printf("OK");
        _free(err);
        return 0;
    }
    else if (res){
        if (!success) {
            printf("Should have Failed");
           _free(res);
           return -1;
        }
        printf("OK");
        _free(res);
        return 0;
    }
    else {
        printf("NO Error and no Result");
        return -1;
    }


}

int runRequests(char *name, int test_index, int mem_track)
{
        int res=0;
        char* content = readContent(name);
        char temp[200];
        if (content==NULL) 
            return -1;

        // create client        

        // TODO init the nodelist
        json_parsed_t* parsed = parse_json(content); 
        if (!parsed) {
            free(content);
            ERROR("Error parsing the requests");
            return -1;
        }


        // parse the data;
        int i;
        char* descr;
        d_token_t *t = NULL, *tests, *test;
        d_token_t *tokens = NULL;

        int failed = 0;

        if ((tests = d_get(parsed->items,key("tests")))) {
            for (i=0, test=tests+1;i<d_len(tests);i++, test=d_next(test)) {
                if (test_index>0 && i+1!=test_index) continue;

                if ((descr=d_get_string(test,"descr")))
                   strcpy(temp,descr);
                else
                   sprintf(temp,"Request #%i",i+1);
                 printf("\n%2i/%2i : %-60s ",i+1,d_len(tests),temp);
                 mem_reset(mem_track);

                 in3_t *c = in3_new();
                 int j;
                 c->max_attempts=1;
                 c->transport = send_mock;
                 for (j=0;j<c->serversCount;j++) 
                     c->servers[j].needsUpdate=false;


                 int fail =execRequest(c, test);
                 if (fail) failed++;



                 in3_free(c);

                 if (mem_get_memleak_cnt()) {
                     printf(" -- Memory Leak detected by malloc #%i!",mem_get_memleak_cnt());
                     if (!fail) failed++;
                 }

                 size_t max_heap=mem_get_max_heap();
                 str_range_t res_size = d_to_json(_tmp_response);
                 bytes_builder_t* bb = bb_new();

                 d_serialize_binary(bb,_tmp_response);

                 _tmp_response = NULL;
                 _tmp_str = NULL;

                 printf(" ( heap: %zu json: %lu bin: %i) ",max_heap, res_size.len, bb->b.len );
                 bb_free(bb);
            }

        }

        free(content);
        for (i=0;i<parsed->len;i++) {
          if (parsed->items[i].data!=NULL && d_type(parsed->items+i)<2) 
             free(parsed->items[i].data);
        }
        free(parsed->items);
        free(parsed);



        printf("\n%2i of %2i successfully tested", d_len(tests)-failed, d_len(tests));

        if (failed) {
           printf("\n%2i tests failed", failed);
           res = failed;
        }
           printf("\n");

        return failed;

}


int main(int argc, char *argv[])
{
    in3_register_eth_full();
    return runRequests(argv[1], argc>2 ? atoi(argv[2]) : -1,  argc>3 ? atoi(argv[3]) : -1 );
}