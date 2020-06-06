/// checking the transaction data

#include <in3/btc_api.h>
#include <in3/client.h> // the core client
#include <in3/in3_init.h>
#include <in3/utils.h>
#include <stdio.h>

int main() {
  // create new incubed client for BTC
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_BTC);

  // the hash of transaction that we want to get
  bytes32_t tx_id;
  hex_to_bytes("c41eee1c2d97f6158ea3b3aeba0a5271a2174067a38d089ccc1eefbc796706e0", -1, tx_id, 32);

  // fetch and verify the transaction
  btc_transaction_t* tx = btc_get_transaction(in3, tx_id);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!tx)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    // we loop through the tx outputs
    for (int i = 0; i < tx->vout_len; i++)
      // and prrint the values
      printf("Transaction vout #%d : value: %d\n", i, tx->vout[i].value);
    free(tx);
  }

  // cleanup client after usage
  in3_free(in3);
}
