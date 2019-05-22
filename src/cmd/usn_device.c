/** @file 
 * simple commandline-util sending in3-requests.
 * */
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <client/cache.h>
#include <client/client.h>
#include <eth_full.h>
#include <evm.h>
#include <in3_curl.h>
#include <in3_storage.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <usn_api.h>
#include <util/data.h>
#include <util/debug.h>
#include <util/utils.h>

uint64_t getChainId(char* name) {
  if (strcmp(name, "mainnet") == 0) return 0x01L;
  if (strcmp(name, "kovan") == 0) return 0x2aL;
  if (strcmp(name, "tobalaba") == 0) return 0x44dL;
  if (strcmp(name, "evan") == 0) return 0x4b1L;
  if (strcmp(name, "evan") == 0) return 0x4b1L;
  if (strcmp(name, "ipfs") == 0) return 0x7d0;
  return atoi(name);
}

static int handle_booking(usn_event_t* ev) {
  printf("\n%s Booking timestamp=%" PRIu64 "\n", ev->type == BOOKING_START ? "START" : "STOP", ev->ts);
  return 0;
}

int main(int argc, char* argv[]) {
  //  newp();
  int i;
  if (argc < 2) {
    fprintf(stdout, "Usage: %s <options> method params ... \n  -p -proof    none|standard|full\n  -s -signs    number of signatures\n  -c -chain    mainnet|kovan|evan|tobalaba|ipfs\n", argv[0]);
    return 1;
  }

  in3_storage_handler_t storage_handler;
  storage_handler.get_item = storage_get_item;
  storage_handler.set_item = storage_set_item;

  in3_register_eth_full();

  in3_t* c = in3_new();
  if (c == NULL) {
    printf("Failed to init client!\n");
    exit(EXIT_FAILURE);
  }

  c->transport    = send_curl;
  c->requestCount = 1;
  c->cacheStorage = &storage_handler;
  in3_cache_init(c);

  usn_device_conf_t usn;
  usn.booking_handler = handle_booking;
  usn.c               = c;
  usn.chain_id        = c->chainId;
  usn.devices         = NULL;
  usn.len_devices     = 0;
  usn.now             = 0;
  memset(usn.contract, 0, 20);

  unsigned int wait_time = 5;

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-adr") == 0)
      hex2byte_arr(argv[++i], -1, usn.contract, 20);
    else if (strcmp(argv[i], "-w") == 0)
      wait_time = atoi(argv[++i]);
    else if (strcmp(argv[i], "-chain") == 0 || strcmp(argv[i], "-c") == 0)
      usn.chain_id = c->chainId = getChainId(argv[++i]);
    else if (strcmp(argv[i], "-debug") == 0)
      c->evm_flags = EVM_PROP_DEBUG;
    else if (strcmp(argv[i], "-signs") == 0 || strcmp(argv[i], "-s") == 0)
      c->signatureCount = atoi(argv[++i]);
    else if (strcmp(argv[i], "-proof") == 0 || strcmp(argv[i], "-p") == 0) {
      if (strcmp(argv[i + 1], "none") == 0)
        c->proof = PROOF_NONE;
      else if (strcmp(argv[i + 1], "standard") == 0)
        c->proof = PROOF_STANDARD;
      else if (strcmp(argv[i + 1], "full") == 0)
        c->proof = PROOF_FULL;
      else {
        printf("Invalid Argument for proof: %s\n", argv[i + 1]);
        return 1;
      }
      i++;
    } else {
      usn_register_device(&usn, argv[i]);
    }
  }

  printf("\n start watching...\n");

  while (true) {
    printf("checking...\n");
    usn.now              = time(NULL);
    unsigned int timeout = usn_update_state(&usn, wait_time) * 1000;
//    printf("\n sleeping...\n");
#if defined(_WIN32) || defined(WIN32)
    Sleep(timeout);
#else
    nanosleep((const struct timespec[]){{0, timeout * 1000000L}}, NULL);
#endif
  }
}
