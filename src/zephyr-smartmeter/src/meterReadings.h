// meterReadings.h
#include <stdint.h>
#include "meterReadingTypes.h"

#if !defined(_METERREADINGS_H_)
#define _METERREADINGS_H_


  typedef struct
  {
    int nExecResult;
    char strVersion[50];
  } getContractVersion_RSP_t;

  extern void                         meterReadingsSetIN3(in3_t* pIN3);
  extern getContractVersion_RSP_t*    meterReadings_getContractVersion();
  extern getReading_RSP_t*            meterReadings_getLastReading();
  extern getReading_RSP_t*            meterReadings_getReading(uint32_t ixReading);
  extern addReading_RSP_t*            meterReadings_addReading( // returns (uint _readingID)
                                                                char*     _timestampYYYYMMDDhhmmss,
                                                                int32_t   _voltage_mV,
                                                                int32_t   _current_mA,
                                                                uint32_t  _counter_mWh
                                                              );
      

#endif // _METERREADINGS_H_
