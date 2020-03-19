/// a example how to watch usn events and act upon it.

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <in3/signer.h>   // signer-api
#include <in3/usn_api.h>
#include <in3/utils.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

static int handle_booking(usn_event_t* ev) {
  printf("\n%s Booking timestamp=%" PRIu64 "\n", ev->type == BOOKING_START ? "START" : "STOP", ev->ts);
  return 0;
}

int main(int argc, char* argv[]) {
  // create new incubed client
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // switch to goerli
  c->chain_id = 0x5;

  // setting up a usn-device-config
  usn_device_conf_t usn;
  usn.booking_handler    = handle_booking;                                          // this is the handler, which is called for each rent/return or start/stop
  usn.c                  = c;                                                       // the incubed client
  usn.chain_id           = c->chain_id;                                             // the chain_id
  usn.devices            = NULL;                                                    // this will contain the list of devices supported
  usn.len_devices        = 0;                                                       // and length of this list
  usn.now                = 0;                                                       // the current timestamp
  unsigned int wait_time = 5;                                                       // the time to wait between the internval
  hex_to_bytes("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, usn.contract, 20); // address of the usn-contract, which we copy from hex

  // register a usn-device
  usn_register_device(&usn, "office@slockit");

  // now we run en endless loop which simply wait for events on the chain.
  printf("\n start watching...\n");
  while (true) {
    usn.now              = time(NULL);                               // update the timestamp, since this is running on embedded devices, this may be depend on the hardware.
    unsigned int timeout = usn_update_state(&usn, wait_time) * 1000; // this will now check for new events and trigger the handle_booking if so.

    // sleep
#if defined(_WIN32) || defined(WIN32)
    Sleep(timeout);
#else
    nanosleep((const struct timespec[]){{0, timeout * 1000000L}}, NULL);
#endif
  }

  // clean up
  in3_free(c);
  return 0;
}