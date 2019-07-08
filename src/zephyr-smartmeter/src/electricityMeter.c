#include "electricityMeter.h"
#include <string.h>
#include "in3_comm_esp32.h"

// fwd. decl.
static void ReadOut_Dummy(getReading_RSP_t* reading);


// ---

typedef void (*fncReadOut_t)(getReading_RSP_t* reading);

fncReadOut_t fncReadOut = in3_comm_esp32_Modbus_ReadOut;




getReading_RSP_t* electricityMeter_ReadOut(){

    static getReading_RSP_t readingRSP;
    memset(&readingRSP, 0, sizeof(readingRSP));

    // read from xyz (what is defined/set above)
    fncReadOut(&readingRSP);

    return &readingRSP;
}


static void ReadOut_Dummy(getReading_RSP_t* reading){
    if (reading){
        reading->nExecResult = 0; //ok
        strncpy(reading->readingEntry.timestampYYYYMMDDhhmmss, "1234567890", sizeof(reading->readingEntry.timestampYYYYMMDDhhmmss));
        reading->readingEntry.i32Voltage_mV = 1;
        reading->readingEntry.i32Current_mA = 2;
        reading->readingEntry.u32EnergyMeter_mWh = 3;
    }
}
