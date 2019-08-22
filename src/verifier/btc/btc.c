#include "../../core/util/utils.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

static void print_hex(char* prefix, uint8_t* data, int len) {
  printf("%s0x", prefix);
  for (int i = 0; i < len; i++) printf("%02x", data[i]);
  printf("\n");
}
static void print(char* prefix, bytes_t data, char* type) {
  uint8_t tmp[32];
  if (strcmp(type, "hash") == 0) {
    rev_copy(tmp, data.data);
    data.data = tmp;
  }

  if (strcmp(type, "int") == 0) {
    printf("%s%i\n", prefix, le_to_int(data.data));
    return;
  }

  print_hex(prefix, data.data, data.len);
}

int main() {

  char* block_hex = BLOCK_B;

  uint8_t block_data[strlen(block_hex) / 2];
  bytes_t block = bytes(block_data, strlen(block_hex) / 2);
  hex2byte_arr(block_hex, -1, block.data, block.len);

  print("version    = ", btc_block_get(block, BTC_B_VERSION), "hex");
  print("parent_hash= ", btc_block_get(block, BTC_B_PARENT_HASH), "hash");
  print("merkle_root= ", btc_block_get(block, BTC_B_MERKLE_ROOT), "hash");
  print("timestamp  = ", btc_block_get(block, BTC_B_TIMESTAMP), "int");
  print("bits       = ", btc_block_get(block, BTC_B_BITS), "hex");
  print("nonce      = ", btc_block_get(block, BTC_B_NONCE), "hex");

  bytes32_t hash;
  btc_hash(btc_block_get(block, BTC_B_HEADER), hash);
  print_hex("hash       = ", hash, 32);
  print("header     = ", btc_block_get(block, BTC_B_HEADER), "hex");

  btc_target_le(block, hash);
  print("target     = ", bytes(hash, 32), "hash");

  int tx_count = btc_get_transaction_count(block);
  printf("tx_cnt     = %i\n", tx_count);
  bytes_t transactions[tx_count];
  btc_get_transactions(block, transactions);
  for (int i = 0; i < tx_count; i++) {
    printf(" %i : ", i);
    print("", transactions[i], "hex");
  }
  printf("\n------------------------------------\n");

  bytes32_t tx_hashes[tx_count];

  // now calculate the transactionhashes
  for (int i = 0; i < tx_count; i++) {
    btc_hash(transactions[i], tx_hashes[i]);
    printf(" %i : ", i);
    print("", bytes(tx_hashes[i], 32), "hex");
  }

  bytes32_t root;
  btc_merkle_create_root(tx_hashes, tx_count, root);
  print("root = ", bytes(root, 32), "hex");

  //  print("rest \n", bytes(block.data + 80, block.len - 80), "hex");
}