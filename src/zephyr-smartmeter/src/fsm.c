#include "project.h"
#include <console.h>

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


static char buffer[256];
static unsigned short ixWrite;


void resetReceiveData(){
  memset(buffer, 0, sizeof(buffer));
  ixWrite = 0;
}

int receiveData() {
  char ch;
  printf("pre-console_read\n");
  ssize_t bytesRead = console_read(NULL, &ch, 1);
  printf("post-console_read: %d\n", bytesRead);
  if (bytesRead < 0) {
    if (bytesRead != -EAGAIN && bytesRead != -EBUSY) {
      dbg_log("<--- ERR could not read from console/ser'port: bytesRead = %d\n", bytesRead);
      return -1; // err
    }
  } else if (bytesRead > 0) {
    // switch (ch) {
    //   case '~': {
    //     printf("^^^");
    //   } break;
    //   case '\r': {
    //     printf("$r$\n");
    //   } break;
    //   case '\n': {
    //     printf("$n$\n");
    //   } break;
    //   default: {
    //     printf("%c", ch);
    //   } break;
    // }
    if (ixWrite >= sizeof(buffer)) { // cancel with error, if our buffer is full/too small
      dbg_log("<--- ERR buffer too small\n");
      return -1; // err
    }
    if (ixWrite > 0 || ch == '~') { // we are in "read data"-mode
      buffer[ixWrite++] = ch;
      if (ch == '\n' || ch == '\r')                             // End Of Data
      { 
        dbg_log("<-- received EndOfData\n");
        buffer[ixWrite-1] = '\0';
        return 1; // OK, done receiving data
      }
    }
  }
  return 0;
}

int isReceivedData_Equal(char *strCMP) {
  return strncmp(&buffer[1], strCMP, sizeof(buffer)) == 0;
}

int isReceivedData_StartsWith(char *strCMP) {
  unsigned int nLen = strlen(strCMP);
  if ( nLen > (sizeof(buffer)-1) ) {
    nLen = (sizeof(buffer)-1);
  }
  return memcmp(&buffer[1], strCMP, nLen) == 0;
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
  printf("do_action()\n");
  k_sleep(900);
  static u32_t timeOut = 0;
  // action
  switch (g_activityState)
  {
    case AS_start: // send request (and then wait for "OK connected")
      // printf("~[{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[]}]\n");
      printf("AS_start\n");
      resetReceiveData();
      if (!l_bReady)
      {
        timeOut =  k_uptime_get_32(); // "now!"
        g_activityState = AS_waitFor_Ready;
      } else {
        g_activityState = AS_sendRequest;
      }    
      break;
    case AS_waitFor_Ready:
    {
      printf("AS_waitFor_Ready\n");
      
      printf("pre-receive\n");
      int erg = receiveData();
      printf("post-receive\n");
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
          printf("no data received\n");
          break;
      }

      u32_t now = k_uptime_get_32();
      if (    g_activityState == AS_waitFor_Ready
          &&  now >= timeOut)  // timeout; send again RESET-Cmd.
      {
        printf("~>H\n");
        timeOut = k_uptime_get_32() + 7000;
      } else {
        printf("    now: %u\ntimeout: %u\n", now, timeOut);
      }
        
    } break;
    case AS_sendRequest:
    {
      printf("AS_sendRequest\n");
      resetReceiveData();
      // printf("~[{'jsonrpc':'2.0','method':'eth_blockNumber','params':[]}]\n");
      // ~[{"jsonrpc":"2.0","method":"eth_blockNumber","params":[]}]\n
      printf("~[{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[]}]\n");
      g_activityState = AS_waitFor_OkConnected;
      timeOut = k_uptime_get_32() + 7000;
    } break;
    case AS_waitFor_OkConnected:
    {
      printf("AS_waitFor_OkConnected\n");

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
            timeOut = k_uptime_get_32() + 7000;
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
      printf("AS_waitFor_Response\n");
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
          printf("RESULT:\n");
          printf(&buffer[1]);
          printf("\n\n");
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
  STATE_MAX
} in3_state_t;

typedef in3_state_t in3_state_func_t(void);

static in3_state_t in3_init(void) {
	client = k_calloc(1, sizeof(struct in3_client));
	timer0 = k_calloc(1, sizeof(struct k_timer));
	timer1 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
	timer2 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
	timer3 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
  
  console_init();

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

  printf("in3_waiting()\n");

  k_mutex_lock(&client->mutex, 10000);
  // do client-zeugs
  k_mutex_unlock(&client->mutex);
  wait_for_event();

  // do_action(LOCK .. UNLOCK .. ?);

  if (cntr & 0x01) {
    return STATE_WAITING;
  } else {
    printf("returning \"STATE_ACTION\"\n");
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

in3_state_func_t* const state_table[STATE_MAX] = {
    in3_init,
    in3_waiting,
    in3_action,
    in3_reset,
    NULL
  };

static in3_state_t run_state(in3_state_t state) {
  return state_table[state]();
}

int in3_client_start(void) {
  in3_state_t state = STATE_INIT;
  in3_state_t state_prev = STATE_MAX;

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
    state = run_state(state);
  }
}
