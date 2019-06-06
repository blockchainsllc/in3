#include "../core/client/client.h"
#include "meterReadingTypes.h"

#if !defined(IN3_COMM_ESP32_H)
#define IN3_COMM_ESP32_H

// nrf52 asking esp32 (rs232/serial)
extern int in3_comm_esp32_sendJsonRequestAndWait(
                                    char** urls,
                                    int urls_len, 
                                    char* payload, 
                                    in3_response_t* result
                                );

extern void in3_comm_esp32_Modbus_ReadOut(getReading_RSP_t* reading);

extern char* in3_comm_esp32_getTimestamp();


#endif // IN3_COMM_ESP32_H
