// @PUBLIC_HEADER
/** @file
 * defines the return-values of a function call.
 * */
#ifndef IN3_ERROR_H
#define IN3_ERROR_H

/** ERROR types  used as return values.
 * 
 * All values (except IN3_OK) indicate an error. 
 * 
*/
typedef enum {
  /* On success positive values (impl. defined) upto INT_MAX maybe returned */
  IN3_OK       = 0,   /**< Success */
  IN3_EUNKNOWN = -1,  /**< Unknown error - usually accompanied with specific error msg */
  IN3_ENOMEM   = -2,  /**< No memory */
  IN3_ENOTSUP  = -3,  /**< Not supported */
  IN3_EINVAL   = -4,  /**< Invalid value */
  IN3_EFIND    = -5,  /**< Not found */
  IN3_ECONFIG  = -6,  /**< Invalid config */
  IN3_ELIMIT   = -7,  /**< Limit reached */
  IN3_EVERS    = -8,  /**< Version mismatch */
  IN3_EINVALDT = -9,  /**< Data invalid, eg. invalid/incomplete JSON */
  IN3_EPASS    = -10, /**< Wrong password */
  IN3_ERPC     = -11, /**< RPC error (i.e. in3_ctx_t::error set) */
  IN3_ERPCNRES = -12, /**< RPC no response */
  IN3_EUSNURL  = -13, /**< USN URL parse error */
  IN3_ETRANS   = -14, /**< Transport error */
} in3_ret_t;

// Optional type similar to C++ std::optional
// Optional types must be defined prior to usage (e.g. DEFINE_OPTIONAL_T(int))
// Use OPTIONAL_T_UNDEFINED(t) & OPTIONAL_T_VALUE(t, v) for easy initialization (rvalues)
// Note: Defining optional types for pointers is ill-formed by definition. This is
// because redundant
#define OPTIONAL_T(t) opt_##t
#define DEFINE_OPTIONAL_T(t) \
  typedef struct {           \
    t    value;              \
    bool defined;            \
  } OPTIONAL_T(t)
#define OPTIONAL_T_UNDEFINED(t) ((OPTIONAL_T(t)){.defined = false})
#define OPTIONAL_T_VALUE(t, v) ((OPTIONAL_T(t)){.value = v, .defined = true})

#endif //IN3_ERROR_H
