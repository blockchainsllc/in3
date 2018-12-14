#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "context.h"

#ifndef VERIFIER_H
#define VERIFIER_H



typedef int (*in3_verify)( in3_ctx_t* , in3_chain_t*, jsmntok_t* , in3_request_config_t* , jsmntok_t* );


typedef struct verifier {
   in3_verify verify;
   in3_chain_type_t type;
   struct verifier* next;
} in3_verifier_t;

/*! returns the verifier for the given chainType */
in3_verifier_t* in3_get_verifier(in3_chain_type_t type);
void in3_register_verifier(in3_verifier_t* verifier);


#endif