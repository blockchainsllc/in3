#include "uart_comm.h"
#include <misc/printk.h>
#include <string.h>
#include <uart.h>

#define RXBUF_MAXSIZE	1024
#define DTABUF_MAXSIZE	1024

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
        return -1; // no data
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


int uart0_getNextData(unsigned char* pBuf, int szBuf){
    // returns: -2 .. err; -1 .. no data;  >= 0 szData
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

void resetReceiveData(char* pBuf, int szBuf){
  memset(pBuf, 0, szBuf);
}

/** @brief Receive data from UART
 *
 *  Data is enclosed in <BEGIN_DATA>-symbol '~' and <END_DATA>-symbol '\n'.
 *  Data is then written in the buffer (static/global).
 * 
 *  @return -1 .. error (buffer is full/too small); 0 .. no data ready; 1 .. data available
 */
int receiveData(char* pBuf, int szBuf){
  int retval = 0; // go on reading

  int nLen = uart0_getNextData(pBuf, szBuf);

  if (nLen >= 0)
  { // Data avail.
    retval = 1; 
  } else if (nLen == -1) {
    // no data avail
    retval = 0;
  } else {
    // err
    retval = -1;
  }

  return retval;
}

