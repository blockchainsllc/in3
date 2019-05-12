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
static unsigned int     l_ixWrite = 0;
static UartReadStatus_t l_status = URS_no_data;

static void uart0_isr(struct device *x)
{   
    // unsigned char ch;
	// int nNumBytesRead = 0;
    
    // do {
    //     nNumBytesRead = uart_fifo_read(uart0_dev, &ch, 1);




    // } while nNumBytesRead =

    // if (l_bReceivingData)
    // {
    //     // '\n' in newly read data?
    //     unsigned char* pEnd = memchr(rx_buf + l_ixWrite, '\n', numBytesRead);
    //     if (pEnd == NULL) { // go on reading or "err"
    //         l_ixWrite += numBytesRead;
    //         if (l_ixWrite >= sizeof(rx_buf)){ // err; buffer too small
    //             uart_irq_rx_disable(uart0_dev);
    //             l_status = URS_err; 
    //         } 
    //     } else { // found END_OF_DATA
    //         l_ixWrite += (pEnd - (rx_buf + l_ixWrite));
    //         if (l_ixWrite >= sizeof(data_buf)){ // that can't really happen
    //             l_ixWrite = sizeof(data_buf)-1;
    //         }
    //         // copy data to data buffer
    //         memcpy(data_buf, rx_buf, l_ixWrite);
    //         data_buf[l_ixWrite] = '\0';
    //         l_status = URS_dataReady;
    //     }
        
    // }
    // else
    // {
    //     /* code */
    // }
    
    

	// ARG_UNUSED(x);
	// msg_dump(__func__, rx_buf, len);
    // if (l_ixWrite >= sizeof(rx_buf)){
    //     // error: buffer too small
    // }
}

void uart0_init(void)
{
	uart0_dev = device_get_binding("UART_0");

	// uart_irq_callback_set(uart0_dev, uart0_isr);
	// uart_irq_rx_enable(uart0_dev);
	printk("%s() done\n", __func__);
}

UartReadStatus_t uart_getChar(unsigned char* pZchn){
    UartReadStatus_t readStatus = URS_err;

    printk("pre-uart_poll_in()\n");
    int erg = uart_poll_in(uart0_dev, pZchn);
    printk("post-uart_poll_in()\n");
    switch (erg)
    {
    case -1:// no data available
        readStatus = URS_no_data;
        break;
    case 0:// no data available
        readStatus = URS_dataReady;
        break;
    
    default:
        readStatus = URS_err;
        break;
    }
    return readStatus;
}