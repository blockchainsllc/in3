#include "../../init/in3_init.h"
#include "../../nodeselect/full/nodelist.h"
#include "../../nodeselect/full/nodeselect_def.h"
#include "../../verifier/eth1/nano/rpcs.h"
#include "handlers.h"
#include "helper.h"
#include "req_exec.h"
#include "transport.h"
#include "tx.h"

static void send_request(in3_t* c, int argc, char** argv, char* method, sb_t* args, char** result, char** error) {
  sb_t sb = SB_NULL;
  sb_add_chars(&sb, "{\"method\":\"");
  sb_add_chars(&sb, method);
  sb_add_chars(&sb, "\",\"params\":");
  sb_add_chars(&sb, args->data);
  char* ms_sigs = get_argument(argc, argv, "-sigs", "--ms.signatures", true);
  if (ms_sigs) {
    sb_add_chars(&sb, ",\"in3\":{\"msSigs\":\"");
    sb_add_chars(&sb, ms_sigs);
    sb_add_chars(&sb, "\"}}");
  }
  else
    sb_add_chars(&sb, "}");
  recorder_request(sb.data);

  in3_client_rpc_raw(c, sb.data, result, error);
  if (*result)
    recorder_response(*result);
  else
    recorder_error(*error ? *error : "Error");
  _free(sb.data);
  check_last_output();
#ifdef NODESELECT_DEF
  in3_chain_t*          chain = &c->chain;
  in3_nodeselect_def_t* nl    = in3_nodeselect_def_data(c);
  // Update nodelist if a newer latest block was reported
  if (chain && nl && nl->nodelist_upd8_params && nl->nodelist_upd8_params->exp_last_block) {
    char *r = NULL, *e = NULL;
    if (chain->type == CHAIN_ETH)
      in3_client_rpc(c, FN_ETH_BLOCKNUMBER, "[]", &r, &e);
  }
#endif
}

int main(int argc, char* argv[]) {
  // we want to verify all
  in3_log_set_level(LOG_INFO);

  // define vars
  char*  method = NULL;
  sb_t   args   = SB_NULL;
  in3_t* c      = in3_for_chain(CHAIN_ID_MAINNET);
  sb_add_chars(&args, "[");

  init_transport(c);
  init_recorder(&argc, &argv);
  init_env(c, argc, argv);

  // parse arguments
  for (int i = 1; i < argc; i++) {
    // is it a argument?
    if (configure_arg(c, argv, &i, argc)) continue;

    // now handle arguments for special methods
    if (method == NULL)
      method = argv[i];
    else if (strcmp(method, "sign") == 0 && !get_txdata()->data)
      get_txdata()->data = b_new((uint8_t*) argv[i], strlen(argv[i])); // NOSONAR - this function expects null-terminated string which was checked prior to calling it
    else if (get_txdata()->sig == NULL && (strcmp(method, "call") == 0 || strcmp(method, "send") == 0 || strcmp(method, "abi_encode") == 0 || strcmp(method, "abi_decode") == 0))
      get_txdata()->sig = argv[i];
    else {
      // otherwise we add it to the params
      if (args.len > 1) sb_add_char(&args, ',');
      if (*argv[i] == '-' && *(argv[i] + 1) == 0) {
        bytes_t b = readFile(stdin);
        sb_add_range(&args, (char*) b.data, 0, b.len);
        continue;
      }
      if ((*argv[i] == '.' && *(argv[i] + 1) == '/') || *argv[i] == '/') {
        // looks like a file
        FILE* f = fopen(argv[i], "r");
        if (f) {
          bytes_t b = readFile(f);
          fclose(f);
          sb_add_range(&args, (char*) b.data, 0, b.len);
          continue;
        }
      }
      add_argument(argv[i], &args, c, method);
    }
  }
  sb_add_char(&args, ']');

  // start the server?
  if (!method) check_server(c);

  // handle special cmd-methods
  if (handle_rpc(c, &method, &args, argc, argv)) recorder_exit(0);

  // execute
  in3_log_debug("..sending request %s %s\n", method, args.data);
  char *result = NULL, *error = NULL;
  send_request(c, argc, argv, method, &args, &result, &error);

  if (error)
    die(error);
  else if (!result)
    die("No result");
  else
    display_result(method, result);
  recorder_exit(0);
}