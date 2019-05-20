#if !defined(IN3_COMM_ESP32_H)
#define IN3_COMM_ESP32_H

// nrf52 asking esp32 (rs232/serial)
extern int in3_comm_esp32_sendJsonRequestAndWait(
                                    char** urls,
                                    int urls_len, 
                                    char* pl, 
                                    in3_response_t* result
                                );




#endif // IN3_COMM_ESP32_H
