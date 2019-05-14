#include "uart_comm.h"
#include <misc/printk.h>
#include <string.h>
#include <uart.h>

#define BUF_MAXSIZE	1024
// #define SLEEP_TIME	500

static struct device *uart0_dev;
// static unsigned char rx_buf[BUF_MAXSIZE];
static unsigned char data_buf[BUF_MAXSIZE];


static void msg_dump(const char *s, unsigned char *data, unsigned len)
{
	unsigned i;

	printk("%s: ", s);
	for (i = 0U; i < len; i++) {
		printk("%02x ", data[i]);
	}
	printk("(%u bytes)\n", len);
}

static unsigned char    l_bReceivingData = 0;
static unsigned char    l_bBufferFull = 0;
static unsigned int     l_ixWrite = 0;          // next pos to write to
static UartReadStatus_t l_status = URS_no_data;

// data-packets stored in data_buf, separated by '\0'

// do tx-eval in uart-isr already
static void uart0_isr(struct device *x)
{
    static int cntEnter = 0;
    cntEnter++;
    printk("%s - cntEnter: %d\n", __func__, cntEnter);
    int nNumBytesRead = 0;
    unsigned char *posBeginDataSymbol = NULL;
    unsigned char *posEndDataSymbol = NULL;

    unsigned char *posWrite = NULL;
    int nNumBytesFree = 0;

    while (1)
    {    
        posWrite = (data_buf + l_ixWrite);
        nNumBytesFree = sizeof(data_buf) - l_ixWrite;

        if (nNumBytesFree <= 0){
            uart_irq_rx_disable(uart0_dev);
            l_bBufferFull = 1;
            return;
        }
        nNumBytesRead = uart_fifo_read(uart0_dev, posWrite, nNumBytesFree);
        if (nNumBytesRead <= 0){
            break;
        }
        msg_dump("uart0_isr-fifo_read", posWrite, nNumBytesRead);
        while (nNumBytesRead > 0) {
            if (!l_bReceivingData){ 
                // search for '~' (BEGIN_DATA)
                posBeginDataSymbol = memchr(posWrite, '~', nNumBytesRead);
                // if it has been found: "shift" to l_ixWrite and correct l_ixWrite
                if (posBeginDataSymbol){
                    int nOffset = posBeginDataSymbol - posWrite;
                    nNumBytesRead -= nOffset;
                    memmove(posWrite, posBeginDataSymbol, nNumBytesRead);
                    l_bReceivingData = 1;
                } else {
                    // no BEGIN_DATA-symbol ...  "next-write-pos" unchanged
                    nNumBytesRead = 0;
                }
            }
            if ( l_bReceivingData )
            { 
                // receiving data ... END_DATA-symbol ('\n' or '\r')?
                posEndDataSymbol = memchr(posWrite, '\n', nNumBytesRead);
                if (!posEndDataSymbol){ // maybe '\r'?
                    posEndDataSymbol = memchr(posWrite, '\r', nNumBytesRead);
                }
                if (posEndDataSymbol){
                    l_bReceivingData = 0;
                    posEndDataSymbol[0] = '\0';
                    l_status = URS_dataReady;
                    // posWrite und nNumBytesRead korrigieren
                    unsigned char *posWriteNew = posEndDataSymbol + 1;
                    int nLenAddedToData = posWriteNew - posWrite;
                    nNumBytesRead -= nLenAddedToData;
                    posWrite = posWriteNew;
                } else { 
                    // no END_DATA-symbol ... update "next-write-pos"
                    posWrite += nNumBytesRead;
                    nNumBytesRead = 0;
                }
                
            }
        } // while (!done ...)        
        // update l_ixWrite
        l_ixWrite = posWrite - data_buf;
    }
    msg_dump( "uart0_isr-buffer:", data_buf, l_ixWrite);
    cntEnter--;
}



// static void uart0_isr(struct device *x)
// {
//     unsigned char ch;
// 	int nNumBytesRead = 0;
    
//     do {
//         nNumBytesRead = uart_fifo_read(uart0_dev, &ch, 1);




//     } while nNumBytesRead =

//     if (l_bReceivingData)
//     {
//         // '\n' in newly read data?
//         unsigned char* pEnd = memchr(rx_buf + l_ixWrite, '\n', numBytesRead);
//         if (pEnd == NULL) { // go on reading or "err"
//             l_ixWrite += numBytesRead;
//             if (l_ixWrite >= sizeof(rx_buf)){ // err; buffer too small
//                 uart_irq_rx_disable(uart0_dev);
//                 l_status = URS_err; 
//             } 
//         } else { // found END_OF_DATA
//             l_ixWrite += (pEnd - (rx_buf + l_ixWrite));
//             if (l_ixWrite >= sizeof(data_buf)){ // that can't really happen
//                 l_ixWrite = sizeof(data_buf)-1;
//             }
//             // copy data to data buffer
//             memcpy(data_buf, rx_buf, l_ixWrite);
//             data_buf[l_ixWrite] = '\0';
//             l_status = URS_dataReady;
//         }
        
//     }
//     else
//     {
//         /* code */
//     }
    
    

// 	ARG_UNUSED(x);
// 	msg_dump(__func__, rx_buf, len);
//     if (l_ixWrite >= sizeof(rx_buf)){
//         // error: buffer too small
//     }
// }

// int uart0_isDataAvailable(){
//     return l_status == URS_dataReady;
// }

int uart0_getNextDataSize(){
    if (l_status == URS_dataReady){
        return (strlen(data_buf) - 1);
    } else {
        return -1; // err
    }
}

int uart0_getNextData(unsigned char* pBuf, int szBuf){
    // returns: -1 .. err; >= 0 szData
    int nLen = uart0_getNextDataSize();
    if (nLen >= 0){
        if (nLen < szBuf){
            strcpy(pBuf,&data_buf[1]);
            // move remaining data to start of data_buf
            unsigned char *pNextDataEntry = &data_buf[1] + nLen + 1;
            if (pNextDataEntry >= &data_buf[l_ixWrite] ){
                l_status = URS_no_data;
                l_ixWrite = 0;
            } else {
                int nOffset = pNextDataEntry - data_buf;
                l_ixWrite -= nOffset;
                memmove(data_buf, pNextDataEntry, l_ixWrite);
            }
            if (l_bBufferFull){
                l_bBufferFull = 0;
                uart_irq_rx_enable(uart0_dev);
            }
        } else {
            nLen = -2; // err: buff too small
        }
    }
    return nLen;
}


void uart0_init(void)
{
	uart0_dev = device_get_binding("UART_0");

	uart_irq_callback_set(uart0_dev, uart0_isr);
	uart_irq_rx_enable(uart0_dev);
	printk("%s() done\n", __func__);
}

// UartReadStatus_t uart_getChar(unsigned char* pZchn){
//     UartReadStatus_t readStatus = URS_err;

//     int erg = uart_poll_in(uart0_dev, pZchn);
//     switch (erg)
//     {
//     case -1:// no data available
//         readStatus = URS_no_data;
//         break;
//     case 0:// no data available
//         readStatus = URS_dataReady;
//         break;
    
//     default:
//         readStatus = URS_err;
//         break;
//     }
//     return readStatus;
// }