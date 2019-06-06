#include <stdint.h>

#if !defined(_METERREADINGTYPES_H_)
#define _METERREADINGTYPES_H_

  typedef struct 
  {
    char      timestampYYYYMMDDhhmmss[15];
    int32_t   i32Voltage_mV;
    int32_t   i32Current_mA;
    uint32_t  u32Power_mW;
    uint32_t  u32EnergyMeter_mWh;
    uint32_t  u32ADC_14bit;
  }  readingEntry_t;

  typedef struct
  {
    int nExecResult;
    readingEntry_t  readingEntry;
  } getReading_RSP_t;

  typedef struct
  {
    int       nExecResult;
    uint32_t  readingID;
  } addReading_RSP_t;

#endif // _METERREADINGTYPES_H_


