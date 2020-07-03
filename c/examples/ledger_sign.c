
#include <in3/client.h>  // the core client
#include <in3/eth_api.h> // functions for direct api-access
#include <in3/ethereum_apdu_client.h>
#include <in3/in3_init.h>      // if included the verifier will automaticly be initialized.
#include <in3/ledger_signer.h> //to invoke ledger nano device for signing
#include <in3/log.h>           // logging functions
#include <in3/utils.h>
#include <stdio.h>

static void send_tx_api(in3_t* in3);

int main() {
  // create new incubed client
  uint8_t bip_path[5] = {44, 60, 0, 0, 0};
  in3_t*  in3         = in3_for_chain(CHAIN_ID_MAINNET);
  in3_log_set_level(LOG_DEBUG);
  // setting ledger nano s to be the default signer for incubed client
  // it will cause the transaction or any msg to be sent to ledger nanos device for siging
  eth_ledger_set_signer_txn(in3, bip_path);
  // eth_ledger_set_signer(in3, bip_path);

  // send tx using API
  send_tx_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void send_tx_api(in3_t* in3) {
  // prepare parameters
  address_t to, from;
  hex_to_bytes("0xC51fBbe0a68a7cA8d33f14a660126Da2A2FAF8bf", -1, from, 20);
  hex_to_bytes("0xd46e8dd67c5d32be8058bb8eb970870f07244567", -1, to, 20);

  bytes_t* data = hex_to_new_bytes("0x00", 0);
  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_UNDEFINED(uint64_t));

  // if the result is null there was an error and we can get the latest error message from eth_last_error()
  if (!tx_hash)
    printf("error sending the tx : %s\n", eth_last_error());
  else {
    printf("Transaction hash: ");
    b_print(tx_hash);
    b_free(tx_hash);
  }
  b_free(data);
}
