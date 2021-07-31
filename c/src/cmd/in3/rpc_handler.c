#include "../../api/eth1/abi.h"
#include "../../api/eth1/eth_api.h"
#include "../../api/ipfs/ipfs_api.h"
#include "helper.h"
#include "transport.h"
#include "tx.h"
#include "weights.h"
#define CHECK_RPC(name, fn) \
  if (strcmp(*method, name) == 0) return fn;

static bool decode_keystore(char* args, int argc, char** argv) {
  json_ctx_t* ctx = parse_json(args);
  if (d_len(ctx->result) != 1 || d_type(ctx->result + 1) != T_STRING) die("decoding a key expects one argument with the filename of the keystorefile.");
  read_pk(d_get_string_at(ctx->result, 0), get_argument(argc, argv, "-pwd", "--password", true), NULL, "key");
  json_free(ctx);
  return true;
}
static bool _call(in3_t* c, char** method, sb_t* params) {
  encode_abi(c, params, true);
  *method = "eth_call";
  return false;
}

#ifdef MOD_WALLET
void* wallet_get_default(void* conf, int type_mask);
void* wallet_get_config(in3_t* c);
#endif

static bool _send(in3_t* c, char** method, sb_t* params) {
  encode_abi(c, params, false);
  if (is_onlyshow_rawtx() && (c->plugin_acts & (PLGN_ACT_SIGN | PLGN_ACT_SIGN_ACCOUNT)) == 0)
    *method = "in3_prepareTx";
  else
    *method = get_txdata()->wait ? "eth_sendTransactionAndWait" : "eth_sendTransaction";

#ifdef MOD_WALLET
  void* conf = wallet_get_config(c);
  if (conf && wallet_get_default(conf, 0))
    *method = "wallet_exec";
#endif
  return false;
}

static bool _sign(sb_t* params) {
  params->len   = 0;
  bytes_t* data = get_txdata()->data;
  if (data->len > 2 && data->data[0] == '0' && data->data[1] == 'x')
    data = hex_to_new_bytes((char*) data->data + 2, data->len - 2);
  sb_add_rawbytes(params, "[\"0x", *data, 0);
  sb_add_chars(params, "\",NULL,\"");
  sb_add_chars(params, get_txdata()->signtype ? get_txdata()->signtype : "raw");
  sb_add_chars(params, "\"]");
  if (data != get_txdata()->data) b_free(data);
  return false;
}

static bool _autocompletelist() {
  recorder_print(0, "send call abi_encode abi_decode ipfs_get ipfs_put ecrecover key -sigtype -st eth_sign raw hash sign createkey -ri -ro keystore unlock pk2address pk2public mainnet goerli ewc btc ipfs local true false latest -np -debug -c -chain -p -version -proof -s -signs -b -block -to -d -data -gas_limit -value -w -wait -hex -json in3_nodeList in3_stats in3_sign web3_clientVersion web3_sha3 net_version net_peerCount net_listening eth_protocolVersion eth_syncing eth_coinbase eth_mining eth_hashrate eth_gasPrice eth_accounts eth_blockNumber eth_getBalance eth_getStorageAt eth_getTransactionCount eth_getBlockTransactionCountByHash eth_getBlockTransactionCountByNumber eth_getUncleCountByBlockHash eth_getUncleCountByBlockNumber eth_getCode eth_sign eth_sendTransaction eth_sendRawTransaction eth_call eth_estimateGas eth_getBlockByHash eth_getBlockByNumber eth_getTransactionByHash eth_getTransactionByBlockHashAndIndex eth_getTransactionByBlockNumberAndIndex eth_getTransactionReceipt eth_pendingTransactions eth_getUncleByBlockHashAndIndex eth_getUncleByBlockNumberAndIndex eth_getCompilers eth_compileLLL eth_compileSolidity eth_compileSerpent eth_newFilter eth_newBlockFilter eth_newPendingTransactionFilter eth_uninstallFilter eth_getFilterChanges eth_getFilterLogs eth_getLogs eth_getWork eth_submitWork eth_submitHashrate in3_cacheClear\n");
  return true;
}

static bool _abi_encode(sb_t* args) {
  char*       error   = NULL;
  json_ctx_t* in_data = parse_json(args->data);
  if (!in_data) die("invalid params");
  if (!get_txdata()->sig) die("missing signature");
  abi_sig_t* s = abi_sig_create(get_txdata()->sig, &error);
  if (s && !error) {
    bytes_t data = abi_encode(s, in_data->result, &error);
    if (data.data)
      print_hex(data.data, data.len);
    abi_sig_free(s);
  }
  if (error) die(error);
  return true;
}

static bool _abi_decode(sb_t* args) {
  char*       error   = NULL;
  json_ctx_t* in_data = parse_json(args->data);
  if (!in_data) die("invalid params");
  if (!get_txdata()->sig) die("missing signature");
  abi_sig_t* s = abi_sig_create(get_txdata()->sig, &error);
  if (s && !error) {
    bytes_t     data = d_to_bytes(d_get_at(parse_json(args->data)->result, 0));
    json_ctx_t* res  = abi_decode(s, data, &error);
    if (error) die(error);
    if (*get_output_conf() & out_json)
      recorder_print(0, "%s\n", d_create_json(res, res->result));
    else
      print_val(res->result);
    abi_sig_free(s);
  }
  if (error) die(error);
  return true;
}

static bool _ipfs_get(in3_t* c, sb_t* args) {
  if (c->chain.type != CHAIN_IPFS) configure(c, "chainId", "ipfs");
  int size = args->len;
  if (size == 2 || args->data[1] != '"' || size < 20 || strstr(args->data + 2, "\"") == NULL) die("missing ipfs hash");
  args->data[size - 2] = 0;
#ifdef IPFS
  bytes_t* content = ipfs_get(c, args->data + 2);
  if (!content) die("IPFS hash not found!");
  fwrite(content->data, content->len, 1, stdout);
  fflush(stdout);
#else
  die("ipfs is not supported. Please compile with -DIPFS=true");
#endif
  return true;
}

static bool _ipfs_put(in3_t* c, sb_t* args) {
  if (c->chain.type != CHAIN_IPFS) configure(c, "chainId", "ipfs");
  args->len = 0;
  sb_add_rawbytes(args, "[\"0x", readFile(stdin), 0);
  sb_add_chars(args, "\",\"hex\"]");
  return false;
}

bool handle_rpc(in3_t* c, char** method, sb_t* params, int argc, char** argv) {
  CHECK_RPC("key", decode_keystore(params->data, argc, argv))
  CHECK_RPC("call", _call(c, method, params))
  CHECK_RPC("abi_encode", _abi_encode(params))
  CHECK_RPC("abi_decode", _abi_decode(params))
  CHECK_RPC("ipfs_get", _ipfs_get(c, params))
  CHECK_RPC("ipfs_put", _ipfs_put(c, params))
  CHECK_RPC("in3_weights", exec_weights(c))
  CHECK_RPC("send", _send(c, method, params))
  CHECK_RPC("sign", _sign(params))
  CHECK_RPC("autocompletelist", _autocompletelist())
  CHECK_RPC("createKey", (*method = "in3_createKey") == NULL)
  CHECK_RPC("pk2address", (*method = "in3_pk2address") == NULL)
  CHECK_RPC("pk2public", (*method = "in3_pk2public") == NULL)
  CHECK_RPC("ecrecover", (*method = "in3_ecrecover") == NULL)
  if (get_txdata()->wait) {
    CHECK_RPC("eth_sendTransaction", (*method = "eth_sendTransactionAndWait") == NULL)
  }
  return false;
}