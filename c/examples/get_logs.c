///  fetching events and verify them with eth_getLogs

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <inttypes.h>
#include <stdio.h>

static void get_logs_rpc(in3_t* in3);
static void get_logs_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3    = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  in3->chain_id = ETH_CHAIN_ID_KOVAN;

  // get logs using raw RPC call
  get_logs_rpc(in3);

  // get logs using API
  get_logs_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_logs_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,           //  the configured client
      "eth_getLogs", // the rpc-method you want to call.
      "[{}]",        // the arguments as json-string
      &result,       // the reference to a pointer whill hold the result
      &error);       // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Logs : \n%s\n", result);
    free(result);
  } else {
    printf("Error getting logs : \n%s\n", error);
    free(error);
  }
}

void get_logs_api(in3_t* in3) {
  // Create filter options
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x%" PRIx64 "\"}", eth_blockNumber(in3) - 2);
  json_ctx_t* jopt = parse_json(b);

  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);

  // Get logs
  eth_log_t* logs = NULL;
  in3_ret_t  ret  = eth_getFilterLogs(in3, fid, &logs);
  if (ret != IN3_OK) {
    printf("eth_getFilterLogs() failed [%d]\n", ret);
    return;
  }

  // print result
  while (logs) {
    eth_log_t* l = logs;
    printf("--------------------------------------------------------------------------------\n");
    printf("\tremoved: %s\n", l->removed ? "true" : "false");
    printf("\tlogId: %lu\n", l->log_index);
    printf("\tTxId: %lu\n", l->transaction_index);
    printf("\thash: ");
    ba_print(l->block_hash, 32);
    printf("\n\tnum: %" PRIu64 "\n", l->block_number);
    printf("\taddress: ");
    ba_print(l->address, 20);
    printf("\n\tdata: ");
    b_print(&l->data);
    printf("\ttopics[%lu]: ", l->topic_count);
    for (size_t i = 0; i < l->topic_count; i++) {
      printf("\n\t");
      ba_print(l->topics[i], 32);
    }
    printf("\n");
    logs = logs->next;
    free(l->data.data);
    free(l->topics);
    free(l);
  }
  eth_uninstallFilter(in3, fid);
  json_free(jopt);
}
