#ifndef TEST 
   #define TEST
#endif
#include <stdio.h>
#include <check.h>
#include <string.h>
#include <core/client/client.h>
#include <core/client/context.h>
#include <core/util/utils.h>
#include <core/jsmn/jsmnutil.h>
#include <eth_nano/eth_nano.h>

#include <unistd.h>

#define ERROR(s) printf("Error: %s",s)

char* readContent(char* name) {
    char temp[500];
    sprintf(temp, "../test/testdata/%s.json", name);
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
static jsmntok_t* _tmp_response;

static  int send_mock(char** urls,int urls_len, char* payload, in3_response_t* result) {
    // printf("payload: %s\n",payload);
    int i;
    for (i=0;i<urls_len;i++) {
        jsmntok_t* r = ctx_get_array_token(_tmp_response,i);
        sb_add_char( &result->result, '[');
        sb_add_range( &result->result, _tmp_str ,  r->start, r->end - r->start);
        sb_add_char( &result->result, ']');
    }
    return 0;
}



int execRequest(in3_t *c , char* str, jsmntok_t* test) {
    jsmntok_t* request  = ctx_get_token(str,test,"request");
    jsmntok_t* response = ctx_get_token(str,test,"response");
    jsmntok_t* config = ctx_get_token(str,request,"config");
    jsmntok_t* t;

    // configure in3
    if ((t=ctx_get_token(str,config,"requestCount")))
        c->requestCount = ctx_to_int(str, t,1);
    else
        c->requestCount = 1;

    char method[200], params[5000];
    if ((t=ctx_get_token(str,request,"method")))
        ctx_cpy_string(str,t,method);
    else {
        printf("NO METHOD");
        return -1;
    }
    if ((t=ctx_get_token(str,request,"params")))
        ctx_cpy_string(str,t,params);
    else {
        printf("NO PARAM");
        return -1;
    }

    char* res, *err;
    int success =  (t=ctx_get_token(str,test,"success")) ? ctx_to_bool(str,t) : true;

    _tmp_response = response;
    _tmp_str = str;

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
        if ((t=ctx_get_token(str,test,"error")) && strncmp(str+t->start, err, t->end - t->start)!=0) {
                printf("wrong error: %s", err);
               _free(err);
               return -1;
        }
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

int runRequests(char *name)
{
        int res=0;
        char* content = readContent(name);
        char temp[200];
        if (content==NULL) 
            return -1;

        // create client        

        // TODO init the nodelist

        // parse the data;
        int tokc=0,  i;
        jsmntok_t *t = NULL, *tests, *test;
        jsmntok_t *tokens = NULL;


        // parse
        res = jsmnutil_parse_json(content, &tokens, &tokc);
        if (res < 0 || tokc == 0) {
            free(content);
            ERROR("Error parsing the requests");
            return -1;
        }

        int failed = 0;

        if ((tests = ctx_get_token(content,tokens,"tests"))) {
            for (i=0;i<tests->size;i++) {
                test = ctx_get_array_token(tests,i);
                if ((t=ctx_get_token(content,test,"descr")))
                   ctx_cpy_string(content,t,temp);
                else
                   sprintf(temp,"Request #%i",i+1);
                 printf("\n%2i/%2i : %-60s ",i+1,tests->size,temp);
                 in3_t *c = in3_new();
                 int j;
                 c->max_attempts=1;
                 c->transport = send_mock;
                 for (j=0;j<c->serversCount;j++) 
                     c->servers[j].needsUpdate=false;

                 if (execRequest(c, content, test)!=0) failed++;
                 _tmp_response = NULL;
                 _tmp_str = NULL;

                 in3_free(c);
            }

        }

        _free(tokens);


        printf("\n%2i of %2i successfully tested", tests->size-failed, tests->size);

        if (failed) {
           printf("\n%2i tests failed", failed);
           res = failed;
        }
           printf("\n");

        return failed;

}


int main(int argc, char *argv[])
{
    char cwd[1000];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("Current working dir: %s\n", cwd);
   } else {
       perror("getcwd() error");
       return 1;
   }
    in3_register_eth_nano();
    return runRequests(argv[1]);
}