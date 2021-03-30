#include "handlers.h"
#include "helper.h"
#include "req_exec.h"
#include "transport.h"
#include "tx.h"

int main(int argc, char* argv[]) {
  // we want to verify all
  in3_log_set_level(LOG_INFO);

  // define vars
  char*  method = NULL;
  sb_t*  args   = sb_new("[");
  in3_t* c      = in3_for_chain(CHAIN_ID_MAINNET);

  init_transport(c);
  init_recorder(&argc, &argv);
  init_env(c, argc, argv);

  // parse arguments
  for (int i = 1; i < argc; i++) {
    if (configure_arg(c, argv, &i, argc)) continue;
    // now handle arguments for special methods
    if (method == NULL)
      method = argv[i];
    else if (strcmp(method, "sign") == 0 && !get_txdata()->data)
      get_txdata()->data = b_new((uint8_t*) argv[i], strlen(argv[i]));
    else if (get_txdata()->sig == NULL && (strcmp(method, "call") == 0 || strcmp(method, "send") == 0 || strcmp(method, "abi_encode") == 0 || strcmp(method, "abi_decode") == 0))
      get_txdata()->sig = argv[i];
    else {
      // otherwise we add it to the params
      if (args->len > 1) sb_add_char(args, ',');
      if (*argv[i] >= '0' && *argv[i] <= '9' && *(argv[i] + 1) != 'x' && strcmp(method, "in3_toWei") && c->chain.chain_id != CHAIN_ID_BTC)
        sb_print(args, "\"%s\"", get_wei(argv[i]));
      else
        sb_print(args,
                 (argv[i][0] == '{' || argv[i][0] == '[' || strcmp(argv[i], "true") == 0 || strcmp(argv[i], "false") == 0 || (*argv[i] >= '0' && *argv[i] <= '9' && strlen(argv[i]) < 16 && *(argv[i] + 1) != 'x'))
                     ? "%s"
                     : "\"%s\"",
                 strcmp(method, "in3_ens") ? resolve(c, argv[i]) : argv[i]);
    }
  }
  sb_add_char(args, ']');

  if (!method) check_server(c);

  // execute
  char *result = NULL, *error = NULL;

  check_last_output();
}