
#include "comm_serial.h"
#include "../core/client/client.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

const int c_nTIME_OUT = 5;

int waitForResponse(char* responseBuffer, unsigned int bufsize, int nTimeOutInSeconds)
{
    int nBytesReceived = serial_Read(responseBuffer, bufsize);
    while (!nBytesReceived && !(nTimeOutInSeconds == 0) && responseBuffer[nBytesReceived]!='\n')
    {
        sleep(1); // wait/sleep 1 second befor looking up again
        nTimeOutInSeconds--;
        nBytesReceived = serial_Read(responseBuffer, bufsize);        
    }
    return (nBytesReceived == 0 && nTimeOutInSeconds == 0) ? -1 : nBytesReceived;
}


void sendRequestAndWaitForResponse( char* url, char* payload, in3_response_t* r  ) 
{
    if (!serial_IsOpen())
    {
        // serial_Open("/dev/tty.wchusbserial1420");
        serial_Open("/dev/tty.usbserial-00000000");
        serial_SetBaud(115200);
    }
    if (!serial_IsOpen())
    {
      sb_add_chars(&r->error, "in3_comm_serial(): Could not open serial port.");
      return;
    }

	serial_OutputFlush();


	char responseBuffer[4096];
	int bufsize = sizeof(responseBuffer);
	int nBytesReceived = serial_Read(responseBuffer, bufsize);

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

    
	// nBytesReceived = serial_Read(responseBuffer, bufsize);
    // for (int nTimeOutInSeconds = c_nTIME_OUT; (!nBytesReceived && !(nTimeOutInSeconds == 0) && responseBuffer[nBytesReceived]!='\n'); nTimeOutInSeconds--)
    // {
    //     sleep(1); // wait/sleep 1 second befor looking up again
    //     nBytesReceived = serial_Read(responseBuffer, bufsize);        
    // }



	// nBytesReceived = serial_Read(responseBuffer, bufsize);
    // for (int nTimeOutInSeconds = c_nTIME_OUT; (!nBytesReceived && !(nTimeOutInSeconds == 0) && responseBuffer[nBytesReceived]!='\n'); nTimeOutInSeconds--)
    // {
    //     sleep(1); // wait/sleep 1 second befor looking up again
    //     nBytesReceived = serial_Read(responseBuffer, bufsize);        
    // }

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

    int nPos = 0;
    int bFound = 0;
    char* pDataStart = NULL;
    char* pDataEnd = NULL;
    nBytesReceived = 0;
    do
    {
        if ( nPos >= nBytesReceived &&  state != stateSendServerAddr && state != stateSendPath && state != stateSendPayload )
        {
            nBytesReceived = waitForResponse(responseBuffer, bufsize, c_nTIME_OUT);
            nPos = 0;
        }        
        responseBuffer[bufsize-1]='\0';

        switch (state)
        {
            case stateSendServerAddr:
            {
                serial_Write("~U", 2);  // START-MARKER: "transmitting URL-server"
                serial_Write(&url[0], nPosSlash);
                serial_Write("\n",1);   // END-MARKER "transmission (URL-server) complete"
                state = stateWaitForACK;
                stateAfterWait = stateSendPath;
            } break;
            case stateSendPath:
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

                state = stateWaitForACK;
                stateAfterWait = stateSendPayload;
            } break;
            case stateSendPayload:
            {
                serial_Write("~", 1);  // START-MARKER: "transmitting Content"
                serial_Write(payload, strlen(payload));
                serial_Write("\n",1);   // END-MARKER "transmission (Content) complete"

                state = stateWaitForACK;
                stateAfterWait = stateExpectingStartOfData;
            } break;
            case stateWaitForACK:
            {
                // look for '~'
                for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
                {
                    if (responseBuffer[nPos] == '~')
                    {
                        bFound = true;
                        state = stateReadACK;
                        nPos++;
                    }
                }
            }break;
            case stateReadACK:
            {
                // look for '\n'
                for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
                {
                    if (responseBuffer[nPos] == '\n')
                    {
                        bFound = true;
                        state = stateDone;
                    }
                }
                if (bFound)
                {
                    state = (stateAfterWait > stateNone) ? stateAfterWait : stateERR;
                    stateAfterWait = stateNone;
                }
            } break;            

            case stateExpectingStartOfData:
            {
                if (nBytesReceived == -1) 
                {
                    state = stateERR;
                    break;
                }
                // look for '~'
                for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
                {
                    if (responseBuffer[nPos] == '~')
                    {
                        bFound = true;
                        state = stateReadData;
                        nPos++;
                        pDataStart = &responseBuffer[nPos];
                    }
                }
                
            } break;
            case stateReadData:
            {
                pDataEnd = NULL;                        
                // look for '\n'
                for( bFound = 0; !bFound && nPos < nBytesReceived; nPos++)
                {
                    if (responseBuffer[nPos] == '\n')
                    {
                        bFound = true;
                        responseBuffer[nPos]='\0';
                        if (responseBuffer[nPos-1]=='\r')
                        {
                            responseBuffer[nPos-1] = '\0';
                            pDataEnd = &responseBuffer[nPos-1];
                        }
                        state = stateDone;
                    }
                }
                if (!pDataEnd) { pDataEnd = &responseBuffer[nPos]; }
                if ( (pDataEnd-pDataStart) > 0 )
                {
                    sb_add_range(&r->result, pDataStart, 0, (pDataEnd-pDataStart) );
                    printf("%s", pDataStart);
                } 
                else
                {
                    sb_add_chars(&r->error, "in3_comm_serial() failed (nothing received).");
                }
            } break;            
            default:
                break;
        }

    } while (state != stateDone && state != stateERR);
    printf("\n\n");
    

    // serial_Close();
}




// int send_curl(char** urls,int urls_len, char* payload, in3_response_t* result) {
int sendJsonRequest_serial(char** urls,int urls_len, char* payload, in3_response_t* result)
{
// printf("payload: %s\n",payload);
  int i;
  for (i=0;i<urls_len;i++) 
  {
  // printf("  url: %s\n",urls[i]);
    sendRequestAndWaitForResponse(urls[i],payload, result+i );
  }
  return 0;

}
