#ifndef _in3_err_EUNKNOWN_
#define _in3_err_EUNKNOWN_ -1
#endif
#ifndef _in3_err_ENOMEM_
#define _in3_err_ENOMEM_ -2
#endif
#ifndef _in3_err_ENOTSUP_
#define _in3_err_ENOTSUP_ -3
#endif
#ifndef _in3_err_EINVAL_
#define _in3_err_EINVAL_ -4
#endif
#ifndef _in3_err_EFIND_
#define _in3_err_EFIND_ -5
#endif
#ifndef _in3_err_ECONFIG_
#define _in3_err_ECONFIG_ -6
#endif
#ifndef _in3_err_ELIMIT_
#define _in3_err_ELIMIT_ -7
#endif
#ifndef _in3_err_EVERS_
#define _in3_err_EVERS_ -8
#endif
#ifndef _in3_err_EINVALDT_
#define _in3_err_EINVALDT_ -9
#endif
#ifndef _in3_err_EPASS_
#define _in3_err_EPASS_ -10
#endif
#ifndef _in3_err_ERPC_
#define _in3_err_ERPC_ -11
#endif
#ifndef _in3_err_ERPCNRES_
#define _in3_err_ERPCNRES_ -12
#endif
#ifndef _in3_err_EUSNURL_
#define _in3_err_EUSNURL_ -13
#endif
#ifndef _in3_err_ETRANS_
#define _in3_err_ETRANS_ -14
#endif

/** ERROR types  used as return values */
typedef enum in3err {
  /* On success positive values (impl. defined) upto INT_MAX maybe returned */
  IN3_OK       = 0,                  /* Success */
  IN3_EUNKNOWN = _in3_err_EUNKNOWN_, /* Unknown error - usually accompanied with specific error msg */
  IN3_ENOMEM   = _in3_err_ENOMEM_,   /* No memory */
  IN3_ENOTSUP  = _in3_err_ENOTSUP_,  /* Not supported */
  IN3_EINVAL   = _in3_err_EINVAL_,   /* Invalid value */
  IN3_EFIND    = _in3_err_EFIND_,    /* Not found */
  IN3_ECONFIG  = _in3_err_ECONFIG_,  /* Invalid config */
  IN3_ELIMIT   = _in3_err_ELIMIT_,   /* Limit reached */
  IN3_EVERS    = _in3_err_EVERS_,    /* Version mismatch */
  IN3_EINVALDT = _in3_err_EINVALDT_, /* Data invalid, eg. invalid/incomplete JSON */
  IN3_EPASS    = _in3_err_EPASS_,    /* Wrong password */
  IN3_ERPC     = _in3_err_ERPC_,     /* RPC error (i.e. in3_ctx_t::error set) */
  IN3_ERPCNRES = _in3_err_ERPCNRES_, /* RPC no response */
  IN3_EUSNURL  = _in3_err_EUSNURL_,  /* USN URL parse error */
  IN3_ETRANS   = _in3_err_ETRANS_,   /* Transport error */
} in3_error_t;
