
#include "../core/client/client.h"
#include <stdio.h>
// #include <unistd.h>
#include <ctype.h>

#include <kernel.h>
#include <misc/printk.h>
#include "uart_comm.h"
#include "../core/util/debug.h"
#include "in3_comm_esp32.h"
#include <time.h>

#ifdef __ZEPHYR__
  #define printX    printk
  #define fprintX   fprintf   // (kg): fprintk caused link-problems!
  #define snprintX  snprintk
#else
  #define printX    printf
  #define fprintX   fprintf
  #define snprintX  snprintf
#endif

//fwd. decl.
static float hexstrToFloat(char* strHex);


//--- impl.

const int c_nTIME_OUT = 5;


static int serial_Read(void *pBuf, int szBuf)
{
    return receiveData(pBuf, szBuf);
    struct tm timeinfo;
    getLocalTime(&timeinfo);
}

static void serial_Write(const void *pBuf, int szBuf)
{   // data: '\0' is not allowed in data (ptr)
    char *pStr = k_malloc(szBuf+1);
    pStr[szBuf] = '\0';
    memcpy(pStr, pBuf, szBuf);
    printk("%s",pStr);
    k_free(pStr);
}

static unsigned char l_strREQ[16] = {"n/a"};

static int waitForResponse(char* responseBuffer, unsigned int bufsize, int nTimeOutInSeconds)
{
    printk("%s::%s (REQ: %s)\n",__FILE__,__func__, l_strREQ);

    int nBytesReceived = -1; // default: error
    
    int nNumTries = nTimeOutInSeconds * 10; // because of k_sleep(100) later on in the loop
    int bDone = 0;

    while (bufsize > 0 && !bDone && !(nNumTries == 0) )
    {   
        int erg = serial_Read(responseBuffer, bufsize);
        switch (erg)
        {
        case 1: 
        {   // data ready
            // just to be sure: put '\0' at end of buffer
            responseBuffer[bufsize-1] = '\0';
            nBytesReceived = strlen(responseBuffer);
            bDone = 1;
        } break;
        case -1:
        {   // error occured
            nBytesReceived = -2; // err
            bDone = 1;
        } break;
        
        default: // no data ... just keep on waiting
            k_sleep(100);
            nNumTries--;
            nBytesReceived = -1;
            break;
        }
    }
    return nBytesReceived;
}

static char sl_responseBuffer[4096];
static void sendRequestAndWaitForResponse( char* url, char* payload, in3_response_t* r  ) 
{    // we expect here that the printk(..) output goes to the serial port

  printk("### %s::%s:\n", __FILE__, __func__);
  printk("### url: %s\n", url);
  printk("### dta: %s\n\n", payload);

	int bufsize = sizeof(sl_responseBuffer);
	int nBytesReceived = 0;//serial_Read(sl_responseBuffer, bufsize);

    // empty "received-data-queue"
    do 
    {
        nBytesReceived = serial_Read(sl_responseBuffer, bufsize);
    } while (nBytesReceived > 0);


    //  transmit: SERVER, Path (=> URL: Server/Path) and Content
    // we have to split the url ==> server, path

    int nLenURL = strlen(url);
    while( nLenURL > 0 && isspace((int)(*(url+nLenURL-1))) ) 
    {
        nLenURL--;
    }
    url[nLenURL] = '\0';

    if (strncmp("http://", url,7) == 0)
    {
        url += 7;
        nLenURL -= 7;
    } else  if (strncmp("https://", url,8) == 0)
    {
        url += 8;
        nLenURL -= 8;
    }
    int nPosSlash = 0;
    int bFoundSlash = 0;

    while( url[nPosSlash] != '\0')
    {
        if (url[nPosSlash]=='/')
        {
            bFoundSlash = 1;
            break;
        }
        nPosSlash++;
    } 

    // replace all '\n' in the payload by "space" (0x20)
    for(char *pChar = payload; *pChar; pChar++)
    {
        if (*pChar == '\n') *pChar = ' ';
    }

    enum {
        enmStateERR = -1,
        enmStateNone = 0,
        enmStateSendServerAddr,
        enmStateSendPath,
        enmStateSendPayload,
        enmStateWaitForACK,
        enmStateReadData, 
        enmStateDone
    } stateAfterWait = enmStateNone, state = enmStateSendServerAddr;

    u32_t timeOut = 0;
    nBytesReceived = 0;
    do
    {

        switch (state)
        {
            case enmStateSendServerAddr:
            {
                serial_Write("~U", 2);  // START-MARKER: "transmitting URL-server"
                serial_Write(&url[0], nPosSlash);
                serial_Write("\n",1);   // END-MARKER "transmission (URL-server) complete"
                snprintX(l_strREQ,sizeof(l_strREQ),"Set ServerAddr");

                state = enmStateWaitForACK;
                stateAfterWait = enmStateSendPath;
                timeOut =  k_uptime_get_32() + 3000; // "in 3 sec"
            } break;
            case enmStateSendPath:
            {
                serial_Write("~P", 2);  // START-MARKER: "transmitting URL-path"
                if (bFoundSlash)
                {
                    serial_Write(&url[nPosSlash], nLenURL-nPosSlash);
                }
                else
                {
                    serial_Write("/", 1);
                }
                serial_Write("\n",1);   // END-MARKER "transmission (URL-path) complete"

                snprintX(l_strREQ,sizeof(l_strREQ),"Set ServerPath");

                state = enmStateWaitForACK;
                stateAfterWait = enmStateSendPayload;
                timeOut =  k_uptime_get_32() + 3000; // "in 3 sec"
            } break;
            case enmStateSendPayload:
            {
                serial_Write("~", 1);  // START-MARKER: "transmitting Content"
                serial_Write(payload, strlen(payload));
                serial_Write("\n",1);   // END-MARKER "transmission (Content) complete"

                snprintX(l_strREQ,sizeof(l_strREQ),"Request Data (send payload)");

                state = enmStateWaitForACK;
                stateAfterWait = enmStateReadData;
                timeOut =  k_uptime_get_32() + 10000; // "in 10 sec"
            } break;
            case enmStateWaitForACK:
            {
                nBytesReceived = waitForResponse(sl_responseBuffer, bufsize, c_nTIME_OUT);
                sl_responseBuffer[bufsize-1]='\0';
                if (nBytesReceived >= 0)
                {
                    nBytesReceived = 0;
                    if (isReceivedData_StartsWith("OK", sl_responseBuffer, sizeof(sl_responseBuffer))){
                        state = (stateAfterWait > enmStateNone) ? stateAfterWait : enmStateERR;
                        stateAfterWait = enmStateNone;
                        timeOut =  k_uptime_get_32() + 15000; // "in 15 sec"
                    } else if (isReceivedData_StartsWith("ERR", sl_responseBuffer, sizeof(sl_responseBuffer)))
                    {
                        state = enmStateERR;
                        stateAfterWait = enmStateNone;
                    }
                } else {
                    u32_t now = k_uptime_get_32();
                    if (now >= timeOut)  // timeout; abort
                    {
                        printk("abort request\n");
                        state = enmStateERR;
                        stateAfterWait = enmStateNone;
                    }
                }
            }break;
            case enmStateReadData:
            {
                nBytesReceived = waitForResponse(sl_responseBuffer, bufsize, c_nTIME_OUT);
                sl_responseBuffer[bufsize-1]='\0';
                if (nBytesReceived >= 0)
                {
                    sb_add_range(&r->result, sl_responseBuffer, 0, nBytesReceived );
                    nBytesReceived = 0;

                    state = enmStateDone;
                } else {
                    u32_t now = k_uptime_get_32();
                    if (now >= timeOut)  // timeout; abort
                    {
                        printk("abort request\n");
                        state = enmStateERR;
                        stateAfterWait = enmStateNone;
                    }
                }
            } break;            
            case enmStateERR:

            default:
                break;
        }

    } while (state != enmStateDone && state != enmStateERR);
    
}



int in3_comm_esp32_sendJsonRequestAndWait(
                                    char** urls,
                                    int urls_len, 
                                    char* payload, 
                                    in3_response_t* result
                                ) 
{
  int i;
  for (i=0;i<urls_len;i++) 
  {
  // printf("  url: %s\n",urls[i]);
    sendRequestAndWaitForResponse(urls[i],payload, result+i );
  }
  return 0;
}



void in3_comm_esp32_Modbus_ReadOut(getReading_RSP_t* reading){

    if (reading == NULL) return;

    enum {
        enmStateERR = -1,
        enmStateNone = 0,
        enmStateRead_Channel,
        enmStateReadValue,
        enmStateDone
    } stateAfterWait = enmStateNone, state = enmStateRead_Channel;

    int8_t i8Channel = 0; // 0 .. voltage; 1 .. current; 2 .. power; 3 .. energy; 4 .. ADC
    u32_t timeOut = 0;

    char* pNow = in3_comm_esp32_getTimestamp();
    strncpy(reading->readingEntry.timestampYYYYMMDDhhmmss, pNow, sizeof(reading->readingEntry.timestampYYYYMMDDhhmmss));
    reading->readingEntry.timestampYYYYMMDDhhmmss[sizeof(reading->readingEntry.timestampYYYYMMDDhhmmss)-1] = '\0';

	int bufsize = sizeof(sl_responseBuffer);
	int nBytesReceived = 0;
    do
    {
        switch (state)
        {
            case enmStateRead_Channel:
            {
                char cmd[4];
                snprintX(cmd, sizeof(cmd),"~%d\n",i8Channel);
                serial_Write(cmd,sizeof(cmd));   // request channel-x data
                snprintX(l_strREQ,sizeof(l_strREQ),"ESP32 - Reading Ch. %d",i8Channel);

                state = enmStateReadValue;
                stateAfterWait = enmStateNone;
                timeOut =  k_uptime_get_32() + 3000; // "in 3 sec"
            } break;
            case enmStateReadValue:
            {
                nBytesReceived = waitForResponse(sl_responseBuffer, bufsize, c_nTIME_OUT);
                sl_responseBuffer[bufsize-1]='\0';
                if (    nBytesReceived >=3
                    &&  sl_responseBuffer[0] == '0'
                    &&  sl_responseBuffer[1] == 'x'
                    )
                {   // seems to be hex-value
                    nBytesReceived = 0;
                    printk("### %s: channel_%d => %s\n", __func__, i8Channel, sl_responseBuffer);
                    if (isReceivedData_StartsWith("ERR", sl_responseBuffer, sizeof(sl_responseBuffer)))
                    {
                        state = enmStateERR;
                        stateAfterWait = enmStateNone;
                    } else {
                        switch (i8Channel)
                        {
                        case 0: {// voltage
                            reading->nExecResult = 0;
                            reading->readingEntry.i32Voltage_mV = strtol(&sl_responseBuffer[2],NULL,16);
                            float fVoltage = *((float*)(&(reading->readingEntry.i32Voltage_mV)));
                            printf("### Voltage as float: %f\n",fVoltage);
                            reading->readingEntry.i32Voltage_mV = (1000.0 * hexstrToFloat(sl_responseBuffer));
                        } break;
                        case 1: // current
                            reading->nExecResult = 0;
                            reading->readingEntry.i32Current_mA = (1000.0 * hexstrToFloat(sl_responseBuffer));
                            break;
                        case 2: // power
                            reading->nExecResult = 0;
                            reading->readingEntry.u32Power_mW = (1000.0 * hexstrToFloat(sl_responseBuffer));
                            break;
                        case 3: // energy
                            reading->nExecResult = 0;
                            reading->readingEntry.u32EnergyMeter_mWh = (1000.0 * hexstrToFloat(sl_responseBuffer));
                            break;
                        case 4: // ADC
                            reading->nExecResult = 0;
                            reading->readingEntry.u32ADC_14bit = strtol(&sl_responseBuffer[2],NULL,16);
                            state = enmStateDone;
                            break;                        
                        default:
                            state = enmStateERR;
                            break;
                        }
                        if (state != enmStateDone && state != enmStateERR)
                        {
                            //next channel to read
                            state = enmStateRead_Channel;
                            i8Channel++;
                        }
                    }         
                } else {
                    u32_t now = k_uptime_get_32();
                    if ( now >= timeOut )  // timeout; abort
                    {
                        printk("abort request\n");
                        state = enmStateERR;
                        stateAfterWait = enmStateNone;
                    }
                }
            } break;            
            case enmStateERR:
            default:
                break;
        }

    } while (state != enmStateDone && state != enmStateERR);
    if (state == enmStateERR)
    {
        reading->nExecResult = -1;
    }
}

char* in3_comm_esp32_getTimestamp()
{
    static char s_strTimestamp[15];
    memset(s_strTimestamp, 0, sizeof(s_strTimestamp));
    enum {
        enmStateERR = -1,
        enmStateNone = 0,
        enmStateRequestDate,
        enmStateReadDate,
        enmStateDone
    } state = enmStateRequestDate;
    u32_t timeOut = 0;

	int bufsize = sizeof(sl_responseBuffer);
	int nBytesReceived = 0;
    do
    {
        switch (state)
        {
            case enmStateRequestDate:
            {
                char cmd[4];
                snprintX(cmd, sizeof(cmd),"~T\n");
                serial_Write(cmd,sizeof(cmd));   // request channel-x data
                snprintX(l_strREQ,sizeof(l_strREQ),"ESP32 - Reading Time");

                state = enmStateReadDate;
                timeOut =  k_uptime_get_32() + 3000; // "in 3 sec"
            } break;
            case enmStateReadDate:
            {
                nBytesReceived = waitForResponse(sl_responseBuffer, bufsize, c_nTIME_OUT);
                sl_responseBuffer[bufsize-1]='\0';
                if (  nBytesReceived >= 3 )
                {
                    printk("### %s: Timestamp received => %s (len: %d, bytesReceived: %d )\n", __func__, sl_responseBuffer, strlen(sl_responseBuffer), nBytesReceived);
                    if (    nBytesReceived < 14 // timestamp has to be of min. length 14: YYYYMMDDhhmmss   
                        ||  isReceivedData_StartsWith("ERR", sl_responseBuffer, sizeof(sl_responseBuffer)))
                    {
                        state = enmStateERR;
                    } else {
                        strncpy(s_strTimestamp, sl_responseBuffer, sizeof(s_strTimestamp));
                        s_strTimestamp[sizeof(s_strTimestamp)-1] = '\0';
                        state = enmStateDone;
                    }         
                    nBytesReceived = 0;
                } else {
                    u32_t now = k_uptime_get_32();
                    if ( now >= timeOut )  // timeout; abort
                    {
                        printk("abort request\n");
                        state = enmStateERR;
                    }
                }
            } break;            
            case enmStateERR:
            default:
                break;
        }

    } while (state != enmStateDone && state != enmStateERR);
    return s_strTimestamp;
}

static float hexstrToFloat(char* strHex)
{
    int pos = 0;
    if (    strHex[0] == '0' 
        &&  (strHex[0] == 'x' || strHex[0] == 'X') ) 
    {
        pos += 2;
    }
    uint32_t u32Val = strtol(&strHex[pos],NULL,16);
    float fVal = *((float*)(&u32Val));
    return fVal;
}