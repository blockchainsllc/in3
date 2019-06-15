#include "project.h"
#include "../core/util/log.h"
#include <console.h>
#include <misc/byteorder.h>
#include "../eth_basic/eth_basic.h"
#include "uart_comm.h"
#include "in3_comm_esp32.h"
#include "meterReadStorage.h"
// #include "meterReadings.h"
#include "electricityMeter.h"


// Global Client
struct in3_client*  client;
struct k_timer *timer0, *timer1, *timer2, *timer3; // timer structures

// private
static void timer_expired(struct k_timer* work) {
  dbg_log("<--- timer0 expired\n");
  k_sem_give(&client->sem);
}

static void timer1_expired(struct k_timer *work) // one shot timer (monitor led)
{
  dbg_log("<--- timer1 expired\n");
	ledpower_set(IO_ON); // power led on
}

static void timer2_expired(struct k_timer *work) // one shot timer (led stripe)
{
  dbg_log("<--- timer2 expired\n");
	ledstrip_set(IO_OFF); // ledstrip off
}

static void timer3_expired(struct k_timer *work) // one shot timer (lock-coil)
{
  dbg_log("<--- timer3 expired\n");
	lock_set(IO_OFF); // lock off
}

static void wait_for_event(void) {
  if (!client)
    return;

// //  k_sem_take(&client->sem, 600000); // EFnote: 600000 mS = 600 sec = 10 min
//   dbg_log("<--- k_sem_take(&client->sem -- PRE\n");
//   k_sem_take(&client->sem, K_SECONDS(5)); // 60 sec = 1 min
//   dbg_log("<--- k_sem_take(&client->sem -- POST\n");
}

void print_MeterReading(getReading_RSP_t* pReadingResponse) {
  if (pReadingResponse != NULL) {
    if (pReadingResponse->nExecResult >= 0) {
      printk("\tTimeStp: %s\n", pReadingResponse->readingEntry.timestampYYYYMMDDhhmmss);
      printk("\tVoltage: %d mV\n", pReadingResponse->readingEntry.i32Voltage_mV);
      printk("\tCurrent: %d mA\n", pReadingResponse->readingEntry.i32Current_mA);
      printk("\tPower  : %d mW\n", pReadingResponse->readingEntry.u32Power_mW);
      printk("\tEnergy : %d mWh\n", pReadingResponse->readingEntry.u32EnergyMeter_mWh);
    } else {
      printk("\tAn error occured: id %d\n", pReadingResponse->nExecResult);
    }
  }
}

static uint64_t l_u64Cntr = 0;
char* getTimestamp_asString(){
  return in3_comm_esp32_getTimestamp();
}


static char buffer[1024];

typedef enum {
  AS_err = -1,
  AS_done = 0,
  AS_sendRequest,
  AS_start,
  AS_waitFor_Ready,
  AS_waitFor_OkConnected,
  AS_waitFor_Response,
  AS_sleep10s_before_waitForOkConnected,
  AS_callMeterReadings_getContractVersion,
  AS_callMeterReadings_getLastReading,
  AS_callMeterReadings_getLastUnverifiedReadingId,
  AS_callMeterReadings_verify,
  AS_readElectricityMeter,
  AS_callMeterReadings_addReading,
} enmActivityState_t;

// #define AS_AFTER_START  AS_sendRequest
#define AS_AFTER_START  AS_callMeterReadings_getContractVersion

volatile enmActivityState_t g_activityState = 0;

static unsigned char l_bReady = 0;
static int cntErr = 0;
static int32_t l_i32U = 0;
static int32_t l_i32I = 0;
static uint32_t l_u32P = 0;
static getReading_RSP_t* pElectricityMeterReading = NULL;
static uint32_t u32LastUnverifiedReadingId = 0;

// void do_action(action_type_t action)
void do_action()
{
  l_u64Cntr++;
  char bufTmp[21];
  printk("~M%s\n", u64tostr(l_u64Cntr, bufTmp, sizeof(bufTmp)));

  static u32_t timeOut = 0;
  // action
  switch (g_activityState)
  {
    case AS_start: // send request (and then wait for "OK connected")
      // printf("~[{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[]}]\n");
      cntErr = 0;
      resetReceiveData(buffer, sizeof(buffer));
      if (!l_bReady)
      {
        timeOut =  k_uptime_get_32() + 4000; // "in 7 sec"
        g_activityState = AS_waitFor_Ready;
      } else {
        g_activityState = AS_AFTER_START;
      }    
      break;
    case AS_waitFor_Ready:
    {
      
      int erg = receiveData(buffer, sizeof(buffer));
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          printk("<-- err: while receiving data\n");
          break;
        case 1: // data complete
          if (isReceivedData_StartsWith("READY", buffer, sizeof(buffer))){
            l_bReady = 1;
            g_activityState = AS_AFTER_START;
          } else {
            printk("<-- err: received data not equal to READY: \"%s\"\n", &buffer[1]);
            g_activityState = AS_err;
          }
          return;
          break;
      
        default:
          dbg_log("no data received (%d)\n", erg);
          break;
      }

      u32_t now = k_uptime_get_32();
      if (    g_activityState == AS_waitFor_Ready
          &&  now >= timeOut)  // timeout; send again RESET-Cmd.
      {
        printk("~>H\n");
        timeOut = k_uptime_get_32() + 4000;
      } else {
        dbg_log("    now: %u\ntimeout: %u\n", now, timeOut);
      }
      k_sleep(800);    
    } break;
    case AS_callMeterReadings_getContractVersion:
    {
      getContractVersion_RSP_t* pContractVersion_RSP = NULL;
      pContractVersion_RSP = meterReadStorage_getContractVersion();
      if (pContractVersion_RSP != NULL) 
      {
        printk("Contract-version: %s\n", pContractVersion_RSP->strVersion);
      } else {
        printk("An error occured: id %d\n", pContractVersion_RSP->nExecResult);
      }
      g_activityState = AS_callMeterReadings_getLastReading;
      if (pContractVersion_RSP->nExecResult >= 0){
        // ok
        cntErr = 0;
      } else if (++cntErr % 5 == 0) { 
        // 5-times error ==> restart
        l_bReady = 0;
        printk("##### ---- AS_callMeterReadings_getContractVersion - starting after 5x ERR ---- #####\n");
        NVIC_SystemReset();
        g_activityState = AS_start;
      } else {
        printk("Error while AS_callMeterReadings_getContractVersion (cntErr = %d)\n", cntErr);
      }
    } break;
    case AS_callMeterReadings_getLastReading:
    {
      getReading_RSP_t* pReading_RSP = NULL;
      pReading_RSP = meterReadStorage_getLastReading();
      printk("Reading back from Chain:\n");
      print_MeterReading(pReading_RSP);

      if (    pReading_RSP != NULL 
          &&  pReading_RSP->nExecResult >= 0 ) 
      {
        // ok
        cntErr = 0;
        l_i32I = pReading_RSP->readingEntry.i32Current_mA;
        l_i32U = pReading_RSP->readingEntry.i32Voltage_mV;
        l_u32P = pReading_RSP->readingEntry.u32EnergyMeter_mWh;

        // g_activityState = AS_callMeterReadings_addReading;
        g_activityState = AS_readElectricityMeter;
      } else if (++cntErr % 5 == 0) { 
        // 5-times error ==> restart
        printk("##### ---- AS_callMeterReadings_getLastReading - restarting after 5x ERR ---- #####\n");
        l_bReady = 0;
        g_activityState = AS_start;
      } else {
        // error while reading values ... start cycle again with getContractVersion
        printk("Error while AS_callMeterReadings_getLastReading (cntErr = %d)\n", cntErr);
        g_activityState = AS_callMeterReadings_getContractVersion;
      }

    } break;
    case AS_readElectricityMeter:
    {
      printk("Reading Electricity Meter:\n");
      pElectricityMeterReading = electricityMeter_ReadOut();
      print_MeterReading(pElectricityMeterReading);
      if (    pElectricityMeterReading != NULL 
          &&  pElectricityMeterReading->nExecResult >= 0 ) 
      {
        g_activityState = AS_callMeterReadings_addReading;
      } else
      {
        // error while reading values ... start cycle again with getContractVersion
        printk("Error while AS_readElectricityMeter\n");
        g_activityState = AS_callMeterReadings_getContractVersion;
      }
      
    }break;
    case AS_callMeterReadings_addReading:
    {
      addReading_RSP_t *pAddReading_RSP = NULL; 
      pAddReading_RSP = meterReadStorage_addReading( pElectricityMeterReading->readingEntry.timestampYYYYMMDDhhmmss, 
                                                  pElectricityMeterReading->readingEntry.i32Voltage_mV, 
                                                  pElectricityMeterReading->readingEntry.i32Current_mA, 
                                                  pElectricityMeterReading->readingEntry.u32EnergyMeter_mWh 
                                                );

      g_activityState = AS_callMeterReadings_getLastUnverifiedReadingId;      

      if (    pAddReading_RSP != NULL 
          &&  pAddReading_RSP->nExecResult >= 0 ) 
      {
        // ok
        cntErr = 0;
      } else if (++cntErr % 5 == 0) { 
        // 5-times error ==> restart
        l_bReady = 0;
        g_activityState = AS_start;
        printk("##### ---- AS_callMeterReadings_addReading - starting after 5x ERR ---- #####\n");
        // NVC_SystemReset();
      } else {
        printk("Error while calling meterReadings_addReading (cntErr = %d)\n", cntErr);
      }

    }break;
    case AS_callMeterReadings_getLastUnverifiedReadingId:
    {
      getLastUnverifiedReadingId_RSP_t* pLastUnverifiedReadingId_RSP = NULL;
      pLastUnverifiedReadingId_RSP = meterReadStorage_getLastUnverifiedReadingId();

      g_activityState = AS_callMeterReadings_getContractVersion;      

      if (    pLastUnverifiedReadingId_RSP != NULL 
          &&  pLastUnverifiedReadingId_RSP->nExecResult >= 0 ) 
      { // ok
        cntErr = 0;
        int bUnverifiedReadingAvail = pLastUnverifiedReadingId_RSP->bUnverifiedReadingAvailable;
        
        if ( pLastUnverifiedReadingId_RSP->readingID <= u32LastUnverifiedReadingId )
        {
          bUnverifiedReadingAvail =  0;
        }

        if (bUnverifiedReadingAvail) 
        {
          u32LastUnverifiedReadingId  = pLastUnverifiedReadingId_RSP->readingID;
        }

        printk("#### meterReadStorage_getLastUnverifiedReadingId(): readingID = %d, do_verifiy = %d\n", u32LastUnverifiedReadingId, bUnverifiedReadingAvail);
        g_activityState = bUnverifiedReadingAvail ? AS_callMeterReadings_verify : AS_readElectricityMeter; //AS_callMeterReadings_getContractVersion;      
      } else if (++cntErr % 5 == 0) { 
        // 5-times error ==> restart
        l_bReady = 0;
        g_activityState = AS_start;
        printk("##### ---- AS_callMeterReadings_getLastUnverifiedReadingId - starting after 5x ERR ---- #####\n");
        // NVC_SystemReset();
      } else {
        printk("Error while calling meterReadStorage_getLastUnverifiedReadingId (cntErr = %d)\n", cntErr);
      }
    } break;
    case AS_callMeterReadings_verify:
    { // meterReadStorage_verify(uint32_t  _meterReadId)
      printk("#### AS_callMeterReadings_verify: verifying #%d\n", u32LastUnverifiedReadingId);
      int nExecResult = meterReadStorage_verify(u32LastUnverifiedReadingId);

      g_activityState = AS_callMeterReadings_getLastUnverifiedReadingId;      

      if ( nExecResult >= 0 ) 
      { // ok
        cntErr = 0;
        printk("#### AS_callMeterReadings_verify(): executed OK\n");
      } else if (++cntErr % 5 == 0) { 
        // 5-times error ==> restart
        l_bReady = 0;
        g_activityState = AS_start;
        printk("##### ----- AS_callMeterReadings_verify - restarting after 5x ERR ---- #####\n");
        // NVC_SystemReset();
      } else {
        printk("Error while AS_callMeterReadings_verify (cntErr = %d)\n", cntErr);
      }
    } break;
    case AS_sendRequest:
    {
      dbg_log("AS_sendRequest\n");
      resetReceiveData(buffer, sizeof(buffer));
      printk("~{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_call\",\"params\":[{\"from\":\"0x0000000000000000000000000000000000000000\",\"to\":\"0x5bed06b162100cfa567d103795ea55c9368b6c06\",\"data\":\"0x0087a7880000000000000000000000000000000000000000000000000000000000000005\",\"value\":\"0x0\",\"gas\":\"0x2dc6c0\"}]}\n");

      g_activityState = AS_waitFor_OkConnected;
      timeOut = k_uptime_get_32() + 10000;
    } break;
    case AS_waitFor_OkConnected:
    {
      dbg_log("AS_waitFor_OkConnected\n");

      int erg = receiveData(buffer, sizeof(buffer));
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          dbg_log("<-- err: while receiving data\n");
          return;
          break;
        case 1: // data complete
          if (isReceivedData_StartsWith("OK connected", buffer, sizeof(buffer))){
            dbg_log("<-- ok: received data is OK connected\n");
            resetReceiveData(buffer, sizeof(buffer));      
            g_activityState = AS_waitFor_Response;
            timeOut = k_uptime_get_32() + 10000;
          } else {
            dbg_log("<-- err: received data not equal to OK connected: \"%s\"\n", &buffer[1]);
            g_activityState = AS_err;
          }
          return;
          break;
      
        default:
          break;
      }
      if (    g_activityState == AS_waitFor_OkConnected
          &&  k_uptime_get_32() >= timeOut)  // timeout;
      {
        printk("AS_waitFor_OkConnected timed out.\n");
        g_activityState = AS_err; // timeout
      }
      k_sleep(800);    
    } break;
    case AS_waitFor_Response: 
    {
      dbg_log("### AS_waitFor_Response\n");
      int erg = receiveData(buffer, sizeof(buffer));
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          dbg_log("<-- err: while receiving data\n");
          return;
          break;
        case 1: // data complete
          // print result:
          printk("> RESULT:\n>");
          printk(&buffer[0]);
          printk("\n\n");
          g_activityState = AS_done;
          return;
          break;
      
        default:
          break;
      }
      if (    g_activityState == AS_waitFor_Response
          &&  k_uptime_get_32() >= timeOut)  // timeout;
      {
        printk("AS_waitFor_Response timed out.\n");
         g_activityState = AS_err; // timeout
      }
      k_sleep(800);    
    } break;
  
    default:
      k_sleep(800);    
      break;
  }

}
 

// PUBLIC API
void in3_signal_event(void) {
  if (!client)
    return;

  dbg_log("<--- signalling event\n");
  k_sem_give(&client->sem);
}

/////////////////////////////////////

/*******************/
/*  state machine  */
/*******************/

   typedef enum {
        STATE_INIT,
        STATE_WAITING,
        STATE_ACTION,
        STATE_RESET,
        STATE_TEST,
        STATE_MAX
    } in3_state_t;

    typedef in3_state_t in3_state_func_t(void);


static in3_state_t in3_init(void) {
	client = k_calloc(1, sizeof(struct in3_client));
	timer0 = k_calloc(1, sizeof(struct k_timer));
	timer1 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
	timer2 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
	timer3 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
  
  // console_init();
  uart0_init();

	client->in3 = in3_new();

  in3_log_set_quiet(0/*false*/);

	// client->in3->chainId = 0x044d; // Tobalaba
	client->in3->chainId = 0x12046; // Volta
	client->in3->requestCount = 1;
	client->in3->max_attempts=3;
  client->in3->cacheStorage = NULL;
  // g_c->cacheStorage = NULL;
  
  client->in3->proof = PROOF_NONE; //PROOF_STANDARD //proof_standard nur mit eth_full mgl!
  client->in3->use_http = true;

  client->in3->nodeLimit = 5;
  client->in3->autoUpdateList = 0;

  // g_c->transport = sendJsonRequest_serial;
  client->in3->transport = in3_comm_esp32_sendJsonRequestAndWait;


	client->txr = k_calloc(1, sizeof(in3_tx_receipt_t));
	client->msg = k_calloc(1, sizeof(in3_msg_t));

  // in3_cache_init(client->in3);
	in3_register_eth_basic();
	// in3_register_eth_nano();
	// bluetooth_setup(client);

  meterReadStorageSetIN3(client->in3);

	k_sem_init(&client->sem, 0, 1);
	k_mutex_init(&client->mutex);
	k_timer_init(timer0, timer_expired, NULL);
	k_timer_init(timer1, timer1_expired, NULL); // init timer, callback for expired, callback for stopped
	k_timer_init(timer2, timer2_expired, NULL); // init timer, callback for expired, callback for stopped
	k_timer_init(timer3, timer3_expired, NULL); // init timer, callback for expired, callback for stopped

	return STATE_WAITING;
}

static in3_state_t in3_waiting(void) {
  static int cntr = 0;
  cntr++;

  dbg_log("in3_waiting()\n");

  k_mutex_lock(&client->mutex, 10000);
  // do client-zeugs
  k_mutex_unlock(&client->mutex);
  wait_for_event();

  // do_action(LOCK .. UNLOCK .. ?);

  if (cntr & 0x01) {
    return STATE_WAITING;
  } else {
    dbg_log("returning \"STATE_ACTION\"\n");
    return STATE_ACTION;
  }
}

static in3_state_t in3_action(void) {
  // dbg_log("<--- before msg_get_action(..)\n");
  // action_type_t action = msg_get_action(client->msg->data);
  // dbg_log("<--- after msg_get_action(..)\n");
  if (g_activityState <= AS_done) {
    if (g_activityState == AS_err)
    {
      l_bReady = 0; // Reset the ESP32 and wait for "READY"
    }
    g_activityState = AS_start;
  	ledpower_set(IO_OFF); // power led off
  }
  
  do_action(); // changes global variable g_activityState

  if (g_activityState <= AS_done) {
  	ledpower_set(IO_ON); // power led on
    return STATE_WAITING;
  }

  return STATE_ACTION;
}

static in3_state_t in3_reset(void) {
  client->msg->end = k_uptime_get_32();

  // dbg_log("<--- Total time: %lums\n", (unsigned long) client->msg->end - client->msg->start);
  clear_message(client);

  return STATE_WAITING;
}


static void in3_TEST_init(void) {
  printk("in3_TEST_init\n");
  uart0_init();
}

static unsigned int cntr_TEST=0;
static in3_state_t in3_TEST(void) {
  ledpower_set(IO_ON); // power led on
  k_sleep(1000);

//   uart_poll_in(struct device *dev, unsigned char *p_char);
  printk("printk -- cntr = %u\n", cntr_TEST++);

  // unsigned char zchn;
  // UartReadStatus_t readStatus = uart_getChar(&zchn);


  uart0_dumpBuffer();
  extern int l_nNumDataPackets;

  int nLen = uart0_getNextDataSize();
  printk("l_nNumDataPackets: %d, SzNextData: %d\n",l_nNumDataPackets, nLen);
  if (nLen >= 0){
    int szBuf = nLen+1;
    unsigned char *pBuf = k_malloc(nLen+1);
    uart0_getNextData(pBuf, szBuf);
    printk("Found data: %s\n", pBuf);
    k_free(pBuf);
  }

  ledpower_set(IO_OFF); // power led on
  k_sleep(1000);


  return STATE_TEST;
}


in3_state_func_t* const state_table[STATE_MAX] = {
    in3_init,
    in3_waiting,
    in3_action,
    in3_reset,
    in3_TEST,
    NULL
  };

static in3_state_t run_state(in3_state_t state) {
  return state_table[state]();
}

// #define __MY_TEST__


int in3_client_start(void) {

  // in3_state_t state = STATE_TEST;
  in3_state_t state = STATE_INIT;
  in3_state_t state_prev = STATE_MAX;

  printk("in3_client_start\n");
  #ifdef __MY_TEST__
    in3_TEST_init();
  #endif

  // ledpower_set(IO_ON); // power led on
  // k_sleep(3000);
  // ledpower_set(IO_OFF); // power led offs
  // k_sleep(1000);
  // in3_TEST();
  printk("this was just a call to in3_TEST()\n");

  while (1) {
    if (state_prev != state)
    {
      switch(state)
      {
        case STATE_INIT:
          dbg_log("<--- INIT\n");
          break;
        case STATE_WAITING:
          dbg_log("<--- WAITING\n");
          break;
        case STATE_ACTION:
          dbg_log("<--- ACTION\n");
          break;
        case STATE_RESET:
          dbg_log("<--- RESET\n");
          break;
        default:
          dbg_log("<--- STATE MACHINE ERROR!\n");
          state = STATE_RESET; // force state to reset
          break;
      }
    }
    state_prev = state;
    #ifdef __MY_TEST__
      in3_TEST();
    #else
      state = run_state(state);
    #endif
  }
}
