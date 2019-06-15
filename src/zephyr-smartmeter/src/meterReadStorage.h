// meterReadStorage.h
#include <stdint.h>
#include "meterReadingTypes.h"

#if !defined(_METERREADSTORAGE_H_)
#define _METERREADSTORAGE_H_


  typedef struct
  {
    int nExecResult;
    char strVersion[50];
  } getContractVersion_RSP_t;

  extern void                               meterReadStorageSetIN3(in3_t* pIN3);
  extern getContractVersion_RSP_t*          meterReadStorage_getContractVersion();
  extern getReading_RSP_t*                  meterReadStorage_getLastReading();
  extern getReading_RSP_t*                  meterReadStorage_getReading(uint32_t ixReading);
  extern getLastUnverifiedReadingId_RSP_t*  meterReadStorage_getLastUnverifiedReadingId();
  extern int                                meterReadStorage_verify(uint32_t  _meterReadId); 
  extern addReading_RSP_t*                  meterReadStorage_addReading( // returns (uint _readingID)
                                                                char*     _timestampYYYYMMDDhhmmss,
                                                                int32_t   _voltage_mV,
                                                                int32_t   _current_mA,
                                                                uint32_t  _counter_mWh
                                                              );
      

#endif // _METERREADSTORAGE_H_
