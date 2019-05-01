#ifndef in3_comm_serial_h__
#define in3_comm_serial_h__

    #include "../core/client/client.h"

    extern int sendJsonRequest_serial(char** urls,int urls_len, char* payload, in3_response_t* result) ;
 
#endif  // in3_comm_serial_h__
