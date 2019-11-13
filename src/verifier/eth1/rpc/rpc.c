

#include "rpc.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/error.h"
#include "../../../core/util/utils.h"
#include <stdint.h>
#include <string.h>

static verify_res_t report(char* msg, d_token_t* value, char* prefix) {
  char* m = malloc(strlen(msg) + (prefix ? (strlen(prefix) + 1) : 0) + 1);
  if (prefix)
    sprintf(m, "%s %s", prefix, msg);
  else
    memcpy(m, msg, strlen(msg) + 1);
  verify_res_t r = {.src = value ? d_create_json(value) : NULL, .msg = m, .valid = false};
  return r;
}
#define prop_error(msg, ob) report(msg, ob, prop);

#define check_object(name, ob, props)                                          \
  if (d_type(ob) != T_OBJECT) return report(" must be an object", ob, name);   \
  {                                                                            \
    d_token_t* t = (ob);                                                       \
    int        l = d_len(ob);                                                  \
    props;                                                                     \
    if (l) return report(" must not have additional properties!", NULL, name); \
  }

#define required_prop(name, cformat, verify)                         \
  {                                                                  \
    d_token_t* val  = d_get(t, key(name));                           \
    char*      prop = name;                                          \
    if (!val) return report(" is a required property!", NULL, name); \
    l--;                                                             \
    verify_res_t r = cformat(name, val);                             \
    if (!r.valid) return r;                                          \
    if (prop) { verify }                                             \
  }

#define optional_prop(name, cformat, verify) \
  {                                          \
    d_token_t* val  = d_get(t, key(name));   \
    char*      prop = name;                  \
    if (val) {                               \
      l--;                                   \
      verify_res_t r = cformat(name, val);   \
      if (!r.valid) return r;                \
      if (prop) { verify }                   \
    }                                        \
  }

static verify_res_t valid() {
  verify_res_t res = {.src = NULL, .msg = NULL, .valid = true};
  return res;
}

static verify_res_t f_string(char* name, d_token_t* val) {
  if (d_type(val) != T_STRING) return report("must be a string", val, name);
  if (!d_len(val)) return report("can not be empty", val, name);
  return valid();
}

static verify_res_t f_array(char* name, d_token_t* val) {
  if (d_type(val) != T_ARRAY) return report("must be a array", val, name);
  return valid();
}
static verify_res_t f_not_empty(char* name, d_token_t* val) {
  if (!d_len(val)) return report("can not be empty", val, name);
  return valid();
}
static verify_res_t f_any(char* name, d_token_t* val) {
  if (val == NULL) return report("can not be null", val, name);
  return valid();
}
static verify_res_t f_object(char* name, d_token_t* val) {
  if (d_type(val) != T_OBJECT) return report("must be an object", val, name);
  return valid();
}
static verify_res_t f_int(char* name, d_token_t* val) {
  if (d_type(val) != T_INTEGER) return report("must be an integer", val, name);
  return valid();
}
static verify_res_t f_hex(char* name, d_token_t* val) {
  if (d_type(val) != T_INTEGER && d_type(val) != T_BYTES) return report("must be an integer or  bytes", val, name);
  return valid();
}
static verify_res_t f_boolean(char* name, d_token_t* val) {
  if (d_type(val) != T_BOOLEAN) return report("must be an boolean", val, name);
  return valid();
}

verify_res_t verify_rpc_structure(d_token_t* req, d_token_t* response) {

  verify_res_t (*check_fn)(char* name, d_token_t* val) = NULL;
  char* method                                         = NULL;

  check_object("request", req, {
    required_prop("method", f_string, {
      method = d_string(val);
      if (strcmp(method, "eth_blockNumber") == 0)
        check_fn = f_int;
      else
        return prop_error("unsupported method", val);
    });
    required_prop("params", f_array, {});
    required_prop("id", f_not_empty, {
      if (d_type(val) != T_INTEGER && d_type(val) != T_STRING) return prop_error("must be a string or integer", val);
    });
    required_prop("jsonrpc", f_string, {
      if (strcmp(d_string(val), "2.0")) return prop_error("must be '2.0'", val);
    });
    optional_prop("in3", f_object, {
      check_object("in3", val, {
        optional_prop("chainId", f_hex, {});
        optional_prop("includeCode", f_boolean, {});
        optional_prop("verifiedHashes", f_array, {
          for (d_iterator_t iter = d_iter(val); iter.left; d_iter_next(&iter)) {
            if (d_type(iter.token) != T_BYTES || d_len(iter.token) != 32) return prop_error("must be 32bytes hex encoded strings!", iter.token);
          }
        });
        optional_prop("signers", f_array, {
          for (d_iterator_t iter = d_iter(val); iter.left; d_iter_next(&iter)) {
            if (d_type(iter.token) != T_BYTES || d_len(iter.token) != 20) return prop_error("must be 20bytes hex encoded address!", iter.token);
          }
        });
        optional_prop("latestBlock", f_int, {
          if (d_int(val) > 256) return prop_error("must be between 0 and 256", val);
        });
        optional_prop("useRef", f_boolean, {});
        optional_prop("useBinary", f_boolean, {});
        optional_prop("useFullProof", f_boolean, {});
        optional_prop("finality", f_int, {
          if (d_int(val) > 100) return prop_error("must be between 0 and 100%", val);
        });
        optional_prop("verification", f_string, {
          if (strcmp(d_string(val), "never") && strcmp(d_string(val), "proof")) return prop_error("must be either 'never' or 'proof'", val);
        });
      });
    });
  });

  if (response) {
    check_object("response", response, {
      bool err_or_res = false;

      required_prop("id", f_not_empty, {
        if (!d_eq(val, d_get(req, key("id")))) return prop_error("must match the id in the request", val);
      });
      required_prop("jsonrpc", f_string, {
        if (strcmp(d_string(val), "2.0")) return prop_error("must be '2.0'", val);
      }) optional_prop("error", f_not_empty, {
        err_or_res = true;
      });
      optional_prop("result", f_any, {
        if (err_or_res) return prop_error("you cannot have result if there was an error", val);
        err_or_res     = true;
        verify_res_t r = check_fn("result", val);
        if (!r.valid) return r;
      });
      if (!err_or_res) return report("the response must contains either error or result", response, NULL);
    });

    // TODO check response
  }
  return valid();
}