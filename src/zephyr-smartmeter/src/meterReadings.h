// meterReadings.h

#include <stdint.h>

typedef struct 
{
  char      timestampYYYYMMDDhhmmss[15];
  int32_t   i32Voltage_mV;
  int32_t   i32Current_mA;
  uint32_t  u32EnergyMeter_mWh;
}  readingEntry_t;

typedef struct
{
  int nExecResult;
  readingEntry_t  readingEntry;
} getReading_RSP_t;

typedef struct
{
  int nExecResult;
  char strVersion[50];
} getContractVersion_RSP_t;

extern void                         meterReadingsSetIN3(in3_t* pIN3);
extern getContractVersion_RSP_t*    meterReadings_getContractVersion();
extern getReading_RSP_t*            meterReadings_getLastReading();
extern getReading_RSP_t*            meterReadings_getReading(uint32_t ixReading);
extern uint64_t                     meterReadings_addReading(uint48,int24,int24,uint32):(uint256)