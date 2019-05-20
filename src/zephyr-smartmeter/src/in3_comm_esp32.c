
#include "../core/client/client.h"
// #include <stdio.h>
// #include <unistd.h>
#include <ctype.h>

#include <kernel.h>
#include <misc/printk.h>
#include "uart_comm.h"

const int c_nTIME_OUT = 5;


static int serial_Read(void *pBuf, int szBuf)
{
    return receiveData(pBuf, szBuf);
}

static int serial_Write(const void *pBuf, int szBuf)
{   // data: '\0' is not allowed in data (ptr)
    char *pStr = k_malloc(szBuf+1);
    pStr[szBuf] = '\0';
    memcpy(pStr, pBuf, szBuf);
    printk(pStr);
    k_free(pStr);
}


static int waitForResponse(char* responseBuffer, unsigned int bufsize, int nTimeOutInSeconds)
{
    int nBytesReceived = -1; // default: error
    
    int nNumTries = nTimeOutInSeconds * 10; // because of k_sleep(100) later on in the loop
    int bDone = 0;

    while (bufsize > 0 && !bDone && !(nNumTries == 0) )
    {   
        int erg = receiveData(responseBuffer, bufsize);
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
            nBytesReceived = -1; // err
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

static void sendRequestAndWaitForResponse( char* url, char* payload, in3_response_t* r  ) 
{
    printk("%s::%s\n",__FILE__,__func__);
    // we expect here that the printk(..) output goes to the serial port


	static char responseBuffer[4096];
	int bufsize = sizeof(responseBuffer);
	int nBytesReceived = 0;//serial_Read(responseBuffer, bufsize);

    //  transmit: SERVER, Path (=> URL: Server/Path) and Content
    // we have to split the url ==> server, path

    int nLenURL = strlen(url);
    while( nLenURL > 0 && isspace(*(url+nLenURL-1)) ) 
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
        stateERR = -1,
        stateNone = 0,
        stateSendServerAddr,
        stateSendPath,
        stateSendPayload,
        stateWaitForACK,
        stateReadACK,
        stateExpectingStartOfData,
        stateReadData, 
        stateDone
    } stateAfterWait = stateNone, state = stateSendServerAddr;

    // int nPos = 0;
    // int bFound = 0;
    // char* pDataStart = NULL;
    // char* pDataEnd = NULL;
    // nBytesReceived = 0;
    // do
    // {
    //     if (    nPos >= nBytesReceived 
    //         &&  state != stateSendServerAddr 
    //         &&  state != stateSendPath 
    //         &&  state != stateSendPayload )
    //     {
    //         nBytesReceived = waitForResponse(responseBuffer, bufsize, c_nTIME_OUT);
    //         nPos = 0;
    //     }        
    //     responseBuffer[bufsize-1]='\0';

    //     switch (state)
    //     {
    //         case stateSendServerAddr:
    //         {
    //             serial_Write("~U", 2);  // START-MARKER: "transmitting URL-server"
    //             serial_Write(&url[0], nPosSlash);
    //             serial_Write("\n",1);   // END-MARKER "transmission (URL-server) complete"
    //             state = stateWaitForACK;
    //             stateAfterWait = stateSendPath;
    //         } break;
    //         case stateSendPath:
    //         {
    //             serial_Write("~P", 2);  // START-MARKER: "transmitting URL-path"
    //             if (bFoundSlash)
    //             {
    //                 serial_Write(&url[nPosSlash], nLenURL-nPosSlash);
    //             }
    //             else
    //             {
    //                 serial_Write("/", 1);
    //             }
    //             serial_Write("\n",1);   // END-MARKER "transmission (URL-path) complete"

    //             state = stateWaitForACK;
    //             stateAfterWait = stateSendPayload;
    //         } break;
    //         case stateSendPayload:
    //         {
    //             serial_Write("~", 1);  // START-MARKER: "transmitting Content"
    //             serial_Write(payload, strlen(payload));
    //             serial_Write("\n",1);   // END-MARKER "transmission (Content) complete"

    //             state = stateWaitForACK;
    //             stateAfterWait = stateExpectingStartOfData;
    //         } break;
    //         case stateWaitForACK:
    //         {
    //             // look for '~'
    //             for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
    //             {
    //                 if (responseBuffer[nPos] == '~')
    //                 {
    //                     bFound = true;
    //                     state = stateReadACK;
    //                     nPos++;
    //                 }
    //             }
    //         }break;
    //         case stateReadACK:
    //         {
    //             // look for '\n'
    //             for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
    //             {
    //                 if (responseBuffer[nPos] == '\n')
    //                 {
    //                     bFound = true;
    //                     state = stateDone;
    //                 }
    //             }
    //             if (bFound)
    //             {
    //                 state = (stateAfterWait > stateNone) ? stateAfterWait : stateERR;
    //                 stateAfterWait = stateNone;
    //             }
    //         } break;            

    //         case stateExpectingStartOfData:
    //         {
    //             if (nBytesReceived == -1) 
    //             {
    //                 state = stateERR;
    //                 break;
    //             }
    //             // look for '~'
    //             for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
    //             {
    //                 if (responseBuffer[nPos] == '~')
    //                 {
    //                     bFound = true;
    //                     state = stateReadData;
    //                     nPos++;
    //                     pDataStart = &responseBuffer[nPos];
    //                 }
    //             }
                
    //         } break;
    //         case stateReadData:
    //         {
    //             pDataEnd = NULL;                        
    //             // look for '\n'
    //             for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
    //             {
    //                 if (responseBuffer[nPos] == '\n')
    //                 {
    //                     bFound = true;
    //                     responseBuffer[nPos]='\0';
    //                     if (responseBuffer[nPos-1]=='\r')
    //                     {
    //                         responseBuffer[nPos-1] = '\0';
    //                         pDataEnd = &responseBuffer[nPos-1];
    //                     }
    //                     state = stateDone;
    //                 }
    //             }
    //             if (!pDataEnd) { pDataEnd = &responseBuffer[nPos]; }
    //             if ( (pDataEnd-pDataStart) > 0 )
    //             {
    //                 sb_add_range(&r->result, pDataStart, 0, (pDataEnd-pDataStart) );
    //                 printf("%s", pDataStart);
    //             } 
    //             else
    //             {
    //                 sb_add_chars(&r->error, "in3_comm_serial() failed (nothing received).");
    //             }
    //         } break;            
    //         default:
    //             break;
    //     }

    // } while (state != stateDone && state != stateERR);
    // printf("\n\n");
    

    // serial_Close();
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
