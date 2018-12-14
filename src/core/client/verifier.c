#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "verifier.h"

static in3_verifier_t* verifiers=NULL;

void in3_register_verifier(in3_verifier_t* verifier) {
    verifier->next = verifiers;
    verifiers = verifier;
}


in3_verifier_t* in3_get_verifier(in3_chain_type_t type) {
    in3_verifier_t* v = verifiers;
    while (v) {
        if (v->type==type) return v;
        v=v->next;
    }
    return NULL;
}
