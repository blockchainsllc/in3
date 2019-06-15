
#if !defined(_FNCWRAPPERHELPER_H_)
#define _FNCWRAPPERHELPER_H_

#include <abi.h>


typedef void (*CB_extractVal_t)(const char*, void* pUserData);
extern void extract_vals(d_token_t* t, CB_extractVal_t pFncCB, void* pUserData);
extern call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value);


#endif // _FNCWRAPPERHELPER_H_


