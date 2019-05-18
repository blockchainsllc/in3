#include "project.h"
#include <console.h>
#include <misc/byteorder.h>
#include "uart_comm.h"


#define printX printk



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


static char buffer[1024];
static unsigned short ixWrite;

void resetReceiveData(){
  memset(buffer, 0, sizeof(buffer));
}

/** @brief Receive data from UART
 *
 *  Data is enclosed in <BEGIN_DATA>-symbol '~' and <END_DATA>-symbol '\n'.
 *  Data is then written in the buffer (static/global).
 * 
 *  @return -1 .. error (buffer is full/too small); 0 .. no data ready; 1 .. data available
 */
int receiveData(){
  int retval = 0; // go on reading

  int nLen = uart0_getNextData(buffer, sizeof(buffer));

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


int isReceivedData_Equal(char *strCMP) {
  return strncmp(&buffer[0], strCMP, sizeof(buffer)) == 0;
}

int isReceivedData_StartsWith(char *strCMP) {
  unsigned int nLen = strlen(strCMP);
  if ( nLen > (sizeof(buffer)) ) {
    nLen = (sizeof(buffer));
  }
  return memcmp(&buffer[0], strCMP, nLen) == 0;
}


typedef enum {
  AS_err = -1,
  AS_done = 0,
  AS_sendRequest,
  AS_start,
  AS_waitFor_Ready,
  AS_waitFor_OkConnected,
  AS_waitFor_Response,
  AS_sleep10s_before_waitForOkConnected,
} enmActivityState_t;

volatile enmActivityState_t g_activityState = 0;

static unsigned char l_bReady = 0;
// void do_action(action_type_t action)
void do_action()
{
  printX("do_action()\n");
  k_sleep(900);
  static u32_t timeOut = 0;
  // action
  switch (g_activityState)
  {
    case AS_start: // send request (and then wait for "OK connected")
      // printf("~[{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[]}]\n");
      printX("AS_start\n");
      resetReceiveData();
      if (!l_bReady)
      {
        timeOut =  k_uptime_get_32(); // "now!"
        // printX("~>H\n");
        g_activityState = AS_waitFor_Ready;
      } else {
        g_activityState = AS_sendRequest;
      }    
      break;
    case AS_waitFor_Ready:
    {
      printX("AS_waitFor_Ready\n");
      
      int erg = receiveData();
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          dbg_log("<-- err: while receiving data\n");
          break;
        case 1: // data complete
          if (isReceivedData_StartsWith("READY")){
            l_bReady = 1;
            g_activityState = AS_sendRequest;
          } else {
            dbg_log("<-- err: received data not equal to READY: \"%s\"\n", &buffer[1]);
            g_activityState = AS_err;
          }
          break;
      
        default:
          printX("no data received (%d)\n", erg);
          break;
      }

      u32_t now = k_uptime_get_32();
      if (    g_activityState == AS_waitFor_Ready
          &&  now >= timeOut)  // timeout; send again RESET-Cmd.
      {
        printX("~>H\n");
        timeOut = k_uptime_get_32() + 7000;
      } else {
        printX("    now: %u\ntimeout: %u\n", now, timeOut);
      }
        
    } break;
    case AS_sendRequest:
    {
      printX("AS_sendRequest\n");
      resetReceiveData();
      // printX("~[{'jsonrpc':'2.0','method':'eth_blockNumber','params':[]}]\n");
      // ~[{"jsonrpc":"2.0","method":"eth_blockNumber","params":[]}]\n
      printX("~[{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[]}]\n");
      g_activityState = AS_waitFor_OkConnected;
      timeOut = k_uptime_get_32() + 10000;
    } break;
    case AS_waitFor_OkConnected:
    {
      printX("AS_waitFor_OkConnected\n");

      int erg = receiveData();
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          dbg_log("<-- err: while receiving data\n");
          return;
          break;
        case 1: // data complete
          if (isReceivedData_StartsWith("OK connected")){
            dbg_log("<-- ok: received data is OK connected\n");
            resetReceiveData();      
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
         g_activityState = AS_err; // timeout
      }
    } break;
    case AS_waitFor_Response: 
    {
      printX("AS_waitFor_Response\n");
      int erg = receiveData();
      switch (erg)
      {
        case -1: // err
          g_activityState = AS_err;
          dbg_log("<-- err: while receiving data\n");
          return;
          break;
        case 1: // data complete
          // print result:
          printX("RESULT:\n");
          printX(&buffer[1]);
          printX("\n\n");
          g_activityState = AS_done;
          return;
          break;
      
        default:
          break;
      }
      if (    g_activityState == AS_waitFor_Response
          &&  k_uptime_get_32() >= timeOut)  // timeout;
      {
         g_activityState = AS_err; // timeout
      }
    } break;
  
    default:
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
	client->in3->chainId = 0x044d;
	client->in3->requestCount = 1;
	client->in3->max_attempts=1;

	client->txr = k_calloc(1, sizeof(in3_tx_receipt_t));
	client->msg = k_calloc(1, sizeof(in3_msg_t));

	in3_register_eth_nano();
	// bluetooth_setup(client);

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

  printX("in3_waiting()\n");

  k_mutex_lock(&client->mutex, 10000);
  // do client-zeugs
  k_mutex_unlock(&client->mutex);
  wait_for_event();

  // do_action(LOCK .. UNLOCK .. ?);

  if (cntr & 0x01) {
    return STATE_WAITING;
  } else {
    printX("returning \"STATE_ACTION\"\n");
    return STATE_ACTION;
  }
}

static in3_state_t in3_action(void) {
  int           err;
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

  unsigned char recv_char;
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
