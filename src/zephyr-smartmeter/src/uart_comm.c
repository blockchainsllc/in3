#include "uart_comm.h"
#include <misc/printk.h>
#include <string.h>
#include <uart.h>

#define RXBUF_MAXSIZE	1024
#define DTABUF_MAXSIZE	1024
// #define SLEEP_TIME	500

static struct device *uart0_dev;
static unsigned char rx_buf[RXBUF_MAXSIZE];
static unsigned char data_buf[DTABUF_MAXSIZE];


static void msg_dump(const char *s, unsigned char *data, unsigned len)
{
	unsigned i;

	printk("%s: ", s);
	for (i = 0U; i < len; i++) {
		printk("%02x ", data[i]);
	}
	printk("(%u bytes)\n", len);
}

static int l_ixRead = 0;
static int l_ixWrite = 0;
static volatile char l_bNewDataAvailable = 0;

static void uart0_isr(struct device *x){
    ARG_UNUSED(x);
    int nNumBytesFreeContinuous = 0;
    int nNumBytesFree = 0;
    int nNumBytesRead = 0;

    do {
        if (l_ixWrite < l_ixRead) {
            nNumBytesFreeContinuous = l_ixRead - l_ixWrite;
            nNumBytesFree = nNumBytesFreeContinuous ;
        } else {
            nNumBytesFreeContinuous = sizeof(rx_buf) - l_ixWrite;
            nNumBytesFree = nNumBytesFreeContinuous + l_ixRead;
        }

        nNumBytesRead = uart_fifo_read(uart0_dev, &rx_buf[l_ixWrite], nNumBytesFreeContinuous);
        l_ixWrite += nNumBytesRead;
        if (l_ixWrite >= (int)sizeof(rx_buf)) {
            l_ixWrite -= sizeof(rx_buf);
        }
        if (nNumBytesRead > 0 && l_ixWrite == l_ixRead){
            l_ixRead++;
            if (l_ixRead >= (int)sizeof(rx_buf)) {
                l_ixRead -= sizeof(rx_buf);
            }
        }
        l_bNewDataAvailable = l_bNewDataAvailable || nNumBytesRead > 0;
    } while(nNumBytesRead == nNumBytesFreeContinuous);
}


static unsigned char    l_bReceivingData = 0;
static unsigned char    l_bBufferFull = 0;
// static unsigned int     l_ixWrite = 0;          // next pos to write to
static UartReadStatus_t l_status = URS_no_data;

// data-packets stored in data_buf, separated by '\0'

// do tx-eval in uart-isr already
static void uart0_isr_old(struct device *x)
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

static int l_ixWrite_DataBuf = 0;
int l_nNumDataPackets = 0;
static int l_ixStartUnfinishedPacket = -1; // "none" at the beginning

static void copyNewDataToDataBuf(){
    l_bNewDataAvailable = 0;
    int nDataBuf_NumFreeBytes = sizeof(data_buf) - l_ixWrite_DataBuf;
    while (     l_ixRead != l_ixWrite
            &&  nDataBuf_NumFreeBytes > 0 ){
        int nLenCpy = 0;
        if (l_ixRead <= l_ixWrite){
            nLenCpy = min((l_ixWrite - l_ixRead), nDataBuf_NumFreeBytes);
        } else {
            nLenCpy = min( ((int)sizeof(rx_buf) - l_ixRead) , nDataBuf_NumFreeBytes);
        }
        memcpy(&data_buf[l_ixWrite_DataBuf], &rx_buf[l_ixRead], nLenCpy);
        l_ixWrite_DataBuf += nLenCpy;
        l_ixRead += nLenCpy;
        if (l_ixRead >= (int)sizeof(rx_buf)){
            l_ixRead -= sizeof(rx_buf);
        }
        nDataBuf_NumFreeBytes = sizeof(data_buf) - l_ixWrite_DataBuf;
    }
}

static void uart0_parseInput(){
    unsigned char *posBeginDataSymbol = NULL;
    unsigned char *posEndDataSymbol = NULL;

    int ixProcessNext = l_ixWrite_DataBuf;
    copyNewDataToDataBuf();

    // if no new data available: return
    while (ixProcessNext != l_ixWrite_DataBuf){
        // new data
        if (l_ixStartUnfinishedPacket<0){
            // look up for BEGIN_DATA ('~') in new data
            posBeginDataSymbol = memchr(&data_buf[ixProcessNext],'~', l_ixWrite_DataBuf - ixProcessNext);
            if (posBeginDataSymbol){
                // move newly received data beginning with the BEGIN_DATA-Symbol to ixProcessNext
                // update l_ixStartUnfinishedPacket and l_ixWrite_DataBuf
                int ixBeginDataSymbol = posBeginDataSymbol - data_buf;
                memmove(&data_buf[ixProcessNext], posBeginDataSymbol, l_ixWrite_DataBuf - ixBeginDataSymbol);
                l_ixWrite_DataBuf -= (ixBeginDataSymbol - ixProcessNext);
                l_ixStartUnfinishedPacket = ixProcessNext;
            } else {
                l_ixWrite_DataBuf = ixProcessNext;
            }
        }
        if (l_ixStartUnfinishedPacket >= 0){
            // look up for END_DATA ('\r' or '\n') in new data
            posEndDataSymbol = memchr(&data_buf[ixProcessNext],'\n', l_ixWrite_DataBuf - ixProcessNext);
            if (!posEndDataSymbol){ // maybe '\r'?
                posEndDataSymbol = memchr(&data_buf[ixProcessNext],'\r', l_ixWrite_DataBuf - ixProcessNext);
            }
            if (posEndDataSymbol){
                posEndDataSymbol[0] = '\0';
                l_ixStartUnfinishedPacket = -1;
                // update ixProcessNext
                ixProcessNext = (posEndDataSymbol - data_buf) + 1;
                l_nNumDataPackets++;
                l_status = URS_dataReady;
            } else { 
                // no END_DATA-symbol ... update ixProcessNext
                ixProcessNext = l_ixWrite_DataBuf;
            }
        }
    }    
}

int uart0_getNextDataSize(){
    if (l_bNewDataAvailable){
        uart0_parseInput();
    }

    if (l_nNumDataPackets > 0){
        return (strlen(data_buf) - 1);
    } else {
        return -1; // err
    }
}

void uart0_dumpBuffer(){
    if (l_bNewDataAvailable)
    {
        if (l_ixRead > l_ixWrite){
            msg_dump(__func__, &rx_buf[l_ixRead], sizeof(rx_buf) - l_ixRead);
            msg_dump(__func__, &rx_buf[0], l_ixWrite);
        } else {
            msg_dump(__func__, &rx_buf[l_ixRead], l_ixWrite - l_ixRead);
        }
    }
    // l_bNewDataAvailable = 0;
}

void uart0_dumpData(){
    unsigned char *pEndOfString = -1;
    pEndOfString = memchr(data_buf, '\0', l_ixWrite_DataBuf);
        if (l_ixRead > l_ixWrite){
            msg_dump(__func__, &rx_buf[l_ixRead], sizeof(rx_buf) - l_ixRead);
            msg_dump(__func__, &rx_buf[0], l_ixWrite);
        } else {
            msg_dump(__func__, &rx_buf[l_ixRead], l_ixWrite - l_ixRead);
        }
}




int uart0_getNextData(unsigned char* pBuf, int szBuf){
    // returns: -1 .. err; >= 0 szData
    int nLen = uart0_getNextDataSize(); // returns the len (without starting '~'-symbol)
    if (nLen >= 0){
        if (nLen < szBuf){
            strcpy(pBuf,&data_buf[1]);
            // move remaining data to start of data_buf
            unsigned char *pNextDataEntry = &data_buf[1] + nLen + 1;
            int nOffset = pNextDataEntry - data_buf;
            memmove(data_buf, pNextDataEntry, l_ixWrite_DataBuf - nOffset); // '~' + <nLen> + '\n'
            l_ixWrite_DataBuf -= nOffset;
            l_nNumDataPackets--;
            if (l_nNumDataPackets <= 0 ){
                l_status = URS_no_data;
                l_nNumDataPackets = 0;
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