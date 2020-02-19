#ifndef ERROR_H
#define ERROR_H


#define MH_E_NO_ERROR 0
#define MH_E_UNKNOWN_CODE -1
#define MH_E_TOO_SHORT -2
#define MH_E_TOO_LONG -3
#define MH_E_VARINT_NOT_SUPPORTED -4
#define MH_E_DIGSET_TOO_LONG -5

#define MH_E_LAST -5

const char *mh_error_string(int code);

#endif /* end of include guard */
