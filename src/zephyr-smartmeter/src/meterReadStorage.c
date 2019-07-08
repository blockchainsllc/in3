/** @file 
 * Testing wrapper for contract "MeterReadStorage"
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
#include "meterReadStorage.h"
#include "fncWrapperHelper.h"

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

void meterReadStorageSetIN3(in3_t* pIN3){
  l_pIN3 = pIN3;
}

static char* lc_addrContract_tobalaba = "0x5bed06b162100cfa567d103795ea55c9368b6c06"; //  contract address on tobalaba-chain (0x)
static char* lc_addrContract_mainnet = "0xfe992bfe3adf0b61d0ca144fa4c5d6e81385384b";  //  contract address on main-chain (0x01)
static char* lc_addrContract_volta = "0xd97c29adc1780d87a40963ac15c983608b058360";  // contract address (v0.0.2) on volta-chain (0x12046)

#define PK_METER_001_TOBALABA   "F69D0B9DE3E6F7BE20C93B924CED4E009D167A50D321F870430AE9CD312B986C"
#define PK_METER_001_VOLTA      "1A882FD7D65E5541BFE40BDC11B63FD8644132F441BB221B011C7641115DEC9E"

#define PK_METER_001    PK_METER_001_VOLTA

/*
 *     SmartContract "MeterReadStorage" Function-Hashes:
 *     {
 *       "560a1503": "addReading(uint48,int24,int24,uint32)",         <== implemented
 *       "8aa10435": "getContractVersion()",                          <== implemented
 *       "1ab64c77": "getLastReading()",                              <== implemented
 *       "a482917e": "getLastUnverifiedReadingId()",
 *       "41fa41f3": "getNumberOfMeterReads()",
 *       "112f6404": "idOfOldestUnverifiedMeterReading()",
 *       "4afc07f2": "meterReads(uint256)",
 *       "165d428c": "smartMeter()",
 *       "8753367f": "verify(uint256)"
 *     }
*/

static inline char *getContractAddress(){
  char *pstrAddr = "";
  if (l_pIN3) {
    switch (l_pIN3->chainId)
    {
    case 0x01:
      pstrAddr = lc_addrContract_mainnet;
      break;
    case 0x12046:
      pstrAddr = lc_addrContract_volta;
      break;
    case 0xd90f:
    default:
      pstrAddr = lc_addrContract_tobalaba;
      break;
    }
  }
  return pstrAddr;
}

typedef struct 
{
  int ixDataMember;
  getReading_RSP_t* pDataStruct;
} fillDataStruct_t;


static void CB_fillDataStruct(const char* strVal, void* pUserData)
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

static void fillDataStruct(getReading_RSP_t* pReadingRSP, d_token_t* t) 
{
  fillDataStruct_t fillData;
  fillData.ixDataMember = 0;
  fillData.pDataStruct = pReadingRSP;
  
  extract_vals(t, CB_fillDataStruct, &fillData);
}


getLastUnverifiedReadingId_RSP_t* meterReadStorage_getLastUnverifiedReadingId()
{ // function getLastUnverifiedReadingId() public view returns (bool _unverifiedReadingAvailable, uint _id)
  static getLastUnverifiedReadingId_RSP_t getLastUnverifiedReadingId_Response;
  memset(&getLastUnverifiedReadingId_Response, 0, sizeof(getLastUnverifiedReadingId_Response));

  char* block_number  = "latest";
  char* sig           = "getLastUnverifiedReadingId():(bool,uint256)";
  
  char* addrContract  = getContractAddress();
  char* method        = "eth_call";

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  strcpy(paramBuffer, "[]"); // fnc. has no parameters

  call_request_t* req  = NULL;
  req    = prepare_tx(sig, addrContract, paramBuffer, block_number, 0, NULL);

  printk("#### %s: after prepare_tx(..)\n", __func__);
  // send the request
  char* result = NULL;
  char* error = NULL;

  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &result, &error);
  UNUSED_VAR(errID);

  printk("#### %s: after in3_client_rpc(..)\n", __func__);


  if (error) {
    fprintX(stderr, "Error: %s\n", error);
    _free(error);
    getLastUnverifiedReadingId_Response.nExecResult = -1;
    return &getLastUnverifiedReadingId_Response;
  } else {
    printk("#### %s: after in3_client_rpc(..) -- SUCCESS\n", __func__);
    if (result)
    {
      printk("#### %s: after in3_client_rpc(..) -- result:%s\n", __func__, result);

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
          msg_dump("received bytes: ", tmp, l);
          printk("\n#### %s: before req_parse_result(..)\n", __func__);

          if ( l == 64 )
          {
            uint64_t u64UnverifiedReadingAvailable = bytes_to_long(&tmp[0], 32);
            getLastUnverifiedReadingId_Response.bUnverifiedReadingAvailable = u64UnverifiedReadingAvailable;
            uint64_t u64ID = bytes_to_long(&tmp[32], 32);
            getLastUnverifiedReadingId_Response.readingID = u64ID;
          }

          // json_ctx_t* res = req_parse_result(req, bytes(tmp, l));
          // printk("#### %s: after req_parse_result(..) / before fillDataStruct_getLastUnverifiedReadingId\n", __func__);
          // fillDataStruct_getLastUnverifiedReadingId(
          //             &getLastUnverifiedReadingId_Response, 
          //             res->result
          //           );
          printk("#### %s: after value read\n", __func__);

          req_free(req);
        }
        // if not we simply print the result
      }
      _free(result);
    }
  }
  getLastUnverifiedReadingId_Response.nExecResult = 0;
  return &getLastUnverifiedReadingId_Response;
}


getReading_RSP_t* meterReadStorage_getReading(uint32_t ixReading)
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
getReading_RSP_t* meterReadStorage_getLastReading()
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

static void CB_fillContractVersion(const char* strVal, void* pUserData)
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

static void fillContractVersionString(getContractVersion_RSP_t* pRetValContainer, d_token_t* t) 
{
  fillContractVersion_t fillContractVersionString;
  fillContractVersionString.ixDataMember = 0;
  fillContractVersionString.pContractVersion_RSP = pRetValContainer;
  
  extract_vals(t, CB_fillContractVersion, &fillContractVersionString);
}


// function getContractVersion():(string memory)
getContractVersion_RSP_t* meterReadStorage_getContractVersion()
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


addReading_RSP_t* meterReadStorage_addReading(
                                            char*     _timestampYYYYMMDDhhmmss,
                                            int32_t   _voltage_mV,
                                            int32_t   _current_mA,
                                            uint32_t  _counter_mWh)
{
  static addReading_RSP_t addReading_RSP;
  memset(&addReading_RSP, 0, sizeof(addReading_RSP));
  addReading_RSP.nExecResult = -1; // err

  char*     block_number  = NULL;
  uint64_t  gas_limit     = 5000000;
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
  // hex2byte_arr("F69D0B9DE3E6F7BE20C93B924CED4E009D167A50D321F870430AE9CD312B986C", -1, pk, 32); // tobalaba-meter-account
  hex2byte_arr(PK_METER_001, -1, pk, 32); // volta-meter-account

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


int meterReadStorage_verify(uint32_t  _meterReadId)
{  // "8753367f": "verify(uint256):()"
  int nExecResult = -1; // default: err

  char*     block_number  = NULL;
  uint64_t  gas_limit     = 5000000;
  char*     value         = NULL;
  char*     sig           = "verify(uint256)";
  char*     method        = "eth_sendTransaction";
  int       bWait         = 0;//1;
  UNUSED_VAR(bWait);

  char*     addrContract  = getContractAddress();
  bytes32_t pk;

  char paramBuffer[1024]; // supply enough buffer; we use it later on in the prepare_tx!
  memset(paramBuffer, 0, sizeof(paramBuffer));
  snprintX(paramBuffer, sizeof(paramBuffer)-1, "[%u]", _meterReadId);


  // set signer (necessary for method "eth_sendTransaction")
  // hex2byte_arr("F69D0B9DE3E6F7BE20C93B924CED4E009D167A50D321F870430AE9CD312B986C", -1, pk, 32); // tobalaba-meter-account
  hex2byte_arr(PK_METER_001, -1, pk, 32); // volta-meter-account

  eth_set_pk_signer(l_pIN3, pk);

  call_request_t* req = NULL;
  prepare_tx(sig, addrContract, paramBuffer, block_number, gas_limit, value);

  // send the request
  char* pBuffer_Result  = NULL;
  char* error           = NULL;

  // send the request
  int errID = in3_client_rpc(l_pIN3, method, paramBuffer, &pBuffer_Result, &error);
  UNUSED_VAR(errID);


  if (error) {
    printk("### Error: %s\n", error);
    nExecResult = -1;
    // return &getContractVersion_Response;
  } else {
    nExecResult = 0;
    // if (pBuffer_Result) {
    //   // if the result is a string, we remove the quotes
    //   int len     = strlen(pBuffer_Result);
    //   result      = (pBuffer_Result[0] == '"' && pBuffer_Result[len - 1] == '"') ? pBuffer_Result + 1 : pBuffer_Result;
    //   result[ (len -= (pBuffer_Result == result) ? 0:2) ] = '\0';

    //   // if the request was a eth_call, we decode the result
    //   if (req) {
    //     int l = len / 2 - 1;
    //     if (l) {
    //       bytes_t data;
    //       data.data       = _malloc(l + 1);
    //       data.len        = hex2byte_arr(result, -1, data.data, l + 1);
    //       req_parse_result(req, data);
    //       _free(data.data);
    //     }
    //     // if not we simply print the result
    //   }
    // }
  }

  if (req)            req_free(req);
  if (pBuffer_Result) _free(pBuffer_Result);
  if (error)          _free(error);

  return nExecResult;
}

