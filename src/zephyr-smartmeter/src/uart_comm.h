#if !defined(_UART_COMM_H_)
#define _UART_COMM_H_

typedef enum {
    URS_err = -1,
    URS_no_data = 0,
    URS_dataReady,
} UartReadStatus_t;


extern void uart0_init(void);
extern int  uart0_getNextDataSize();
extern int  uart0_getNextData(unsigned char* pBuf, int szBuf);
extern void uart0_dumpBuffer();

extern void resetReceiveData(char* pBuf, int szBuf);
extern int  receiveData(char* pBuf, int szBuf);
extern int isReceivedData_Equal(    char    *strCMP, 
                                    void    *pReceivedDataBuf, 
                                    int     szReceivedDataBuf
                                );
extern int isReceivedData_StartsWith(   char    *strCMP, 
                                        void    *pReceivedDataBuf, 
                                        int     szReceivedDataBuf
                                    );

#endif // _UART_COMM_H_
