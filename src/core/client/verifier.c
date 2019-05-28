#include "verifier.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include "client.h"
#include <stdbool.h>
#include <stdint.h>

static in3_verifier_t* verifiers = NULL;

void in3_register_verifier(in3_verifier_t* verifier) {
  if (in3_get_verifier(verifier->type) != NULL) return;
  verifier->next = verifiers;
  verifiers      = verifier;
}

in3_verifier_t* in3_get_verifier(in3_chain_type_t type) {
  in3_verifier_t* v = verifiers;
  while (v) {
    if (v->type == type) return v;
    v = v->next;
  }
  return NULL;
}

in3_error_t vc_err(in3_vctx_t* vc, char* msg) {
  sb_t* sb = sb_new("[");
  sb_add_hexuint(sb, vc->id);
  sb_add_chars(sb, "]:");
  sb_add_chars(sb, msg);
  (void) ctx_set_error(vc->ctx, sb->data, IN3_EUNKNOWN);
  sb_free(sb);
  return IN3_EUNKNOWN;
}
