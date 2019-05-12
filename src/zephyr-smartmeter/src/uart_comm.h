#if !defined(_UART_COMM_H_)
#define _UART_COMM_H_

typedef enum {
    URS_err = -1,
    URS_no_data,
    URS_dataReady
} UartReadStatus_t;


extern void uart0_init(void);
extern UartReadStatus_t uart_getChar(unsigned char* pZchn);

#endif // _UART_COMM_H_
