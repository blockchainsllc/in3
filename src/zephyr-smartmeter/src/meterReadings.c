/** @file 
 * Testing wrapper for contract "MeterReadings"
 * */

// #include "/opt/zephyr-sdk/arm-zephyr-eabi/arm-zephyr-eabi/include/stdio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <abi.h>
#include <client/cache.h>
#include <client/client.h>
#include <eth_api.h>
#include <eth_full.h>
#include <evm.h>
#include <in3_curl.h>
#include <in3_storage.h>
#include <signer.h>
#include <inttypes.h>
#include <util/debug.h>
#include <util/utils.h>

#include <util/data.h>
#include "meterReadings.h"

#ifdef __ZEPHYR__
  #define printX    printk
  #define fprintX   fprintf   // (kg): fprintk caused link-problems!
  #define snprintX  snprintk
#else
  #define printX    printf
  #define fprintX   fprintf
  #define snprintX  snprintf
#endif



static in3_t* l_pIN3;

void meterReadingsSetIN3(in3_t* pIN3){
  l_pIN3 = pIN3;
}

static char* lc_addrContract_tobalaba = "0x5bed06b162100cfa567d103795ea55c9368b6c06"; // tobalaba-address
static char* lc_addrContract_mainnet = "0xfe992bfe3adf0b61d0ca144fa4c5d6e81385384b";  // main net address

static inline char *getContractAddress(){
  char *pstrAddr = "";
  if (l_pIN3) {
    pstrAddr = l_pIN3->chainId == 0x01L ? lc_addrContract_mainnet : lc_addrContract_tobalaba;
  }
  return pstrAddr;
}

call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value) {
  call_request_t* req = parseSignature(fn_sig);
  if (req->in_data->type == A_TUPLE) {
    json_ctx_t* in_data = parse_json(args);
    if (set_data(req, in_data->result, req->in_data) < 0) { 
      dbg_log("Error: could not set the data"); 
    }
    free_json(in_data);
  }
  sb_t* params = sb_new("");
  sb_add_chars(params, "[{\"to\":\"");
  sb_add_chars(params, to);
  sb_add_chars(params, "\", \"data\":");
  sb_add_bytes(params, "", &req->call_data->b, 1, false);
  if (block_number) {
    sb_add_chars(params, "},\"");
    sb_add_chars(params, block_number);
    sb_add_chars(params, "\"]");
  } else {
    if (value) {
      sb_add_chars(params, ", \"value\":\"");
      sb_add_chars(params, value);
      sb_add_chars(params, "\"");
    }
    sb_add_chars(params, ", \"gasLimit\":");
    uint8_t gasdata[8];
    bytes_t g_bytes = bytes(gasdata, 8);
    long_to_bytes(gas ? gas : 100000, gasdata);
    b_optimize_len(&g_bytes);
    sb_add_bytes(params, "", &g_bytes, 1, false);
    sb_add_chars(params, "}]");
  }
  strcpy(args, params->data);
  sb_free(params);
  return req;
}

typedef void (*CB_extractVal_t)(const char*, void* pUserData);

void extract_vals(d_token_t* t, CB_extractVal_t pFncCB, void* pUserData) {
  char buf[65];
  memset(buf, 0, sizeof(buf));
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT:
      for (d_iterator_t it = d_iter(t); it.left; d_iter_next(&it))
        extract_vals(it.token, pFncCB, pUserData);
      break;
    case T_BOOLEAN:
      snprintX(buf, sizeof(buf),"%s", d_int(t) ? "true" : "false");
      pFncCB(buf, pUserData);
      break;
    case T_INTEGER:
      snprintX(buf, sizeof(buf)-1,"%i", d_int(t));
      pFncCB(buf, pUserData);
      break;
    case T_BYTES:
      if (t->len < 9) {
        // printk("### T_BYTES: %" PRId64 " - ",  d_long(t)); // does not work on nRF52
        char bufTmp[21];
        snprintX(buf, sizeof(buf)-1,"%s",u64tostr(d_long(t), bufTmp, sizeof(bufTmp)));
      } else {
        int pos = 0;
        snprintX(&buf[pos],3,"0x");
        pos += 2;
        for (int i = 0; i < (int)(t->len); i++) 
        {
          snprintX(&buf[pos],3,"%02x", t->data[i]);
          pos += 2;
        }
      }
      pFncCB(buf, pUserData);
      break;
    case T_NULL:
      snprintX(buf, sizeof(buf),"NULL");
      pFncCB(buf, pUserData);
      break;
    case T_STRING:
      snprintX(buf, sizeof(buf),"%s", d_string(t));
      pFncCB(buf, pUserData);
      break;
  }
}

// void CB_printVal(const char* strVal, void* pUserData)
// {
//   dbg_log("%s\n", strVal);
// }

// void print_val(d_token_t* t) 
// {
//   extract_vals(t, CB_printVal, NULL);
// }

typedef struct 
{
  int ixDataMember;
  getReading_RSP_t* pDataStruct;
} fillDataStruct_t;


void CB_fillDataStruct(const char* strVal, void* pUserData)
{
  if (pUserData != NULL)
  {
    fillDataStruct_t *pFillDataStruct = pUserData;
    switch (pFillDataStruct->ixDataMember)
    {
      case 0:
      {
        int nLenMax = sizeof(pFillDataStruct->pDataStruct->readingEntry.timestampYYYYMMDDhhmmss)-1;
        strncpy(pFillDataStruct->pDataStruct->readingEntry.timestampYYYYMMDDhhmmss, strVal, nLenMax);
        pFillDataStruct->pDataStruct->readingEntry.timestampYYYYMMDDhhmmss[nLenMax] = '\0';
      } break;
      case 1:
      {
        pFillDataStruct->pDataStruct->readingEntry.i32Voltage_mV = atoi(strVal);
      } break;
      case 2:
      {
        pFillDataStruct->pDataStruct->readingEntry.i32Current_mA = atoi(strVal);
      } break;
      case 3:
      {
        pFillDataStruct->pDataStruct->readingEntry.u32EnergyMeter_mWh = atoi(strVal);
      } break;  
      default:
        break;
    }
    pFillDataStruct->ixDataMember++;
  }
  else
  {
    printX("%s\n", strVal);
  }
  
}

void fillDataStruct(getReading_RSP_t* pReadingRSP, d_token_t* t) 
{
  fillDataStruct_t fillData;
  fillData.ixDataMember = 0;
  fillData.pDataStruct = pReadingRSP;
  
  extract_vals(t, CB_fillDataStruct, &fillData);
}


getReading_RSP_t* meterReadings_getReading(uint32_t ixReading)
{
  static getReading_RSP_t getReading_Response;
  memset(&getReading_Response, 0, sizeof(getReading_Response));

  char* block_number = "latest";
  char* sig = "getReading(uint256):(uint48,int24,int24,uint32)";
  
  char* addrContract = getContractAddress();

  // const char* paramBuffer = "[0,"Text",234]";
  char* method = "eth_call";

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  snprintX(paramBuffer, sizeof(paramBuffer)-1,"[%u]", ixReading);

  call_request_t* req  = NULL;
  req    = prepare_tx(sig, addrContract, paramBuffer, block_number, 0, NULL);

  // send the request
  char* result = NULL;
  char* error = NULL;

  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &result, &error);
  UNUSED_VAR(errID);


  if (error) {
    fprintX(stderr, "Error: %s\n", error);
    _free(error);
    getReading_Response.nExecResult = -1;
    return &getReading_Response;
  } else {
    if (result)
    {
      // if the result is a string, we remove the quotes
      int len = strlen(result);
      if (result[0] == '"' && result[len - 1] == '"') {
        memmove(result, result + 1, len - 2);
        len -= 2;
        result[len] = 0;
      }

      // if the request was a eth_call, we decode the result
      if (req) {
        int l = len / 2 - 1;
        if (l) {
          uint8_t tmp[l + 1];
          l               = hex2byte_arr(result, -1, tmp, l + 1);
          json_ctx_t* res = req_parse_result(req, bytes(tmp, l));          
          fillDataStruct(&getReading_Response, res->result);

          req_free(req);
        }
        // if not we simply print the result
      }
      _free(result);
    }
  }
  getReading_Response.nExecResult = 0;
  return &getReading_Response;
}


// function getContractVersion():(string,uint)
getReading_RSP_t* meterReadings_getLastReading()
{
  static getReading_RSP_t getLastReading_Response;
  memset(&getLastReading_Response, 0, sizeof(getLastReading_Response));

  char* block_number = "latest";
  char* sig = "getLastReading():(uint48,int24,int24,uint32)";
  // char* sig = "getLastReading():(uint256,int256,int256,uint256)";

  char* addrContract = getContractAddress();

  char* method = "eth_call";

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  strcpy(paramBuffer, "[]");

  call_request_t* req  = NULL;
  req    = prepare_tx(sig, addrContract, paramBuffer, block_number, 0, NULL);

  // send the request
  char* pBuffer_Result = NULL;
  char* result = NULL;
  char* error = NULL;

  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &pBuffer_Result, &error);
  UNUSED_VAR(errID);

  getLastReading_Response.nExecResult = 0;
  if (error) {
    fprintX(stderr, "Error: %s\n", error);
    getLastReading_Response.nExecResult = -1;
    // return &getLastReading_Response;
  } else {
    if (pBuffer_Result) {
      // printf("result: %s\n", result);
      // if the result is a string, we remove the quotes
      int len     = strlen(pBuffer_Result);
      result      = (pBuffer_Result[0] == '"' && pBuffer_Result[len - 1] == '"') ? pBuffer_Result + 1 : pBuffer_Result;
      result[ (len -= (pBuffer_Result == result) ? 0:2) ] = '\0';

      // if the request was a eth_call, we decode the result
      if (req) {
        int l = len / 2 - 1;
        if (l) {
          uint8_t tmp[l + 1];
          l               = hex2byte_arr(result, -1, tmp, l + 1);
          json_ctx_t* res = req_parse_result(req, bytes(tmp, l));
          fillDataStruct(&getLastReading_Response, res->result);
        }
        // if not we simply print the result
      }
    }
  }

  if (req)            req_free(req);
  if (pBuffer_Result) _free(pBuffer_Result);
  if (error)          _free(error);

  return &getLastReading_Response;
}


typedef struct 
{
  int ixDataMember;
  getContractVersion_RSP_t* pContractVersion_RSP;
} fillContractVersion_t;

void CB_fillContractVersion(const char* strVal, void* pUserData)
{
  if (pUserData != NULL)
  {
    fillContractVersion_t *pFillContractVersion = pUserData;
    switch (pFillContractVersion->ixDataMember)
    {
      case 0:
      {
        int nLenMax = sizeof(pFillContractVersion->pContractVersion_RSP->strVersion)-1;
        strncpy(pFillContractVersion->pContractVersion_RSP->strVersion, strVal, nLenMax);
        pFillContractVersion->pContractVersion_RSP->strVersion[nLenMax] = '\0';
      } break;
      default:
        break;
    }
    pFillContractVersion->ixDataMember++;
  }
  else
  {
    printX("%s\n", strVal);
  }
  
}

void fillContractVersionString(getContractVersion_RSP_t* pRetValContainer, d_token_t* t) 
{
  fillContractVersion_t fillContractVersionString;
  fillContractVersionString.ixDataMember = 0;
  fillContractVersionString.pContractVersion_RSP = pRetValContainer;
  
  extract_vals(t, CB_fillContractVersion, &fillContractVersionString);
}


// function getContractVersion():(string memory)
getContractVersion_RSP_t* meterReadings_getContractVersion()
{
  static getContractVersion_RSP_t getContractVersion_Response;
  memset(&getContractVersion_Response, 0, sizeof(getContractVersion_Response));

  char* block_number = "latest";
  char* sig = "getContractVersion():(string)";

  char* addrContract = getContractAddress();

  char* method = "eth_call";

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  strcpy(paramBuffer, "[]");

  call_request_t* req  = NULL;
  req    = prepare_tx(sig, addrContract, paramBuffer, block_number, 0, NULL);

  // send the request
  char* pBuffer_Result = NULL;
  char* result = NULL;
  char* error = NULL;

  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &pBuffer_Result, &error);
  UNUSED_VAR(errID);

  // bool wait = false;
  // // if we need to wait
  // if (!error && result && wait && strcmp(method, "eth_sendTransaction") == 0) {
  //   bytes32_t txHash;
  //   hex2byte_arr(result + 3, 64, txHash, 32);
  //   result = eth_wait_for_receipt(l_pIN3, txHash);
  // }
  getContractVersion_Response.nExecResult = 0;
  if (error) {
    fprintX(stderr, "Error: %s\n", error);
    getContractVersion_Response.nExecResult = -1;
    // return &getContractVersion_Response;
  } else {
    if (pBuffer_Result) {
      // if the result is a string, we remove the quotes
      int len     = strlen(pBuffer_Result);
      result      = (pBuffer_Result[0] == '"' && pBuffer_Result[len - 1] == '"') ? pBuffer_Result + 1 : pBuffer_Result;
      result[ (len -= (pBuffer_Result == result) ? 0:2) ] = '\0';

      // if the request was a eth_call, we decode the result
      if (req) {
        int l = len / 2 - 1;
        if (l) {
          bytes_t data;
          data.data = _malloc(l + 1);
          data.len        = hex2byte_arr(result, -1, data.data, l + 1);
          json_ctx_t* res = req_parse_result(req, data);
          fillContractVersionString(&getContractVersion_Response, res->result);
          _free(data.data);
        }
        // if not we simply print the result
      }
    }
  }

  if (req)            req_free(req);
  if (pBuffer_Result) _free(pBuffer_Result);
  if (error)          _free(error);

  return &getContractVersion_Response;
}


addReading_RSP_t* meterReadings_addReading(
                                            char*     _timestampYYYYMMDDhhmmss,
                                            int32_t   _voltage_mV,
                                            int32_t   _current_mA,
                                            uint32_t  _counter_mWh)
{
  static addReading_RSP_t addReading_RSP;
  memset(&addReading_RSP, 0, sizeof(addReading_RSP));
  addReading_RSP.nExecResult = -1; // err

  char*     block_number  = NULL;
  uint64_t  gas_limit     = 100000;
  char*     value         = NULL;
  char*     sig           = "addReading(uint48,int24,int24,uint32)";
  char*     method        = "eth_sendTransaction";
  int       bWait         = 0;//1;
  UNUSED_VAR(bWait);

  char*     addrContract  = getContractAddress();
  bytes32_t pk;

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  snprintX(paramBuffer, sizeof(paramBuffer)-1, "[%s,%d,%d,%d]", _timestampYYYYMMDDhhmmss, _voltage_mV, _current_mA, _counter_mWh);

  // set signer (necessary for method "eth_sendTransaction")
  hex2byte_arr("F69D0B9DE3E6F7BE20C93B924CED4E009D167A50D321F870430AE9CD312B986C", -1, pk, 32);

  eth_set_pk_signer(l_pIN3, pk);

  call_request_t* req = NULL;
  prepare_tx(sig, addrContract, paramBuffer, block_number, gas_limit, value);

  // send the request
  char* pBuffer_Result  = NULL;
  char* result          = NULL;
  char* error           = NULL;

  // send the request
  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &pBuffer_Result, &error);
  UNUSED_VAR(errID);

  // //  only, if  strcmp(method, "eth_sendTransaction") == 0
  // if (!error && pBuffer_Result && bWait) {
  //   bytes32_t txHash;
  //   hex2byte_arr(pBuffer_Result + 3, 64, txHash, 32);
  //   result = eth_wait_for_receipt(l_pIN3, txHash);
  // }

  // freeing has to be done in "main"-program
  // in3_free(l_pIN3); 

  if (error) {
    printk("### Error: %s\n", error);
    addReading_RSP.nExecResult = -1;
    // return &getContractVersion_Response;
  } else {
    addReading_RSP.nExecResult = 0;
    if (pBuffer_Result) {
      // if the result is a string, we remove the quotes
      int len     = strlen(pBuffer_Result);
      result      = (pBuffer_Result[0] == '"' && pBuffer_Result[len - 1] == '"') ? pBuffer_Result + 1 : pBuffer_Result;
      result[ (len -= (pBuffer_Result == result) ? 0:2) ] = '\0';

      // if the request was a eth_call, we decode the result
      if (req) {
        int l = len / 2 - 1;
        if (l) {
          bytes_t data;
          data.data       = _malloc(l + 1);
          data.len        = hex2byte_arr(result, -1, data.data, l + 1);
          req_parse_result(req, data);
          _free(data.data);
        }
        // if not we simply print the result
      }
    }
  }

  if (req)            req_free(req);
  if (pBuffer_Result) _free(pBuffer_Result);
  if (error)          _free(error);

  return &addReading_RSP;
}

