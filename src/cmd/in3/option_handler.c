#include "../../nodeselect/full/nodelist.h"
#include "../../nodeselect/full/nodeselect_def.h"
#include "../../signer/pk-signer/signer.h"

#include "handlers.h"
#include "helper.h"
#include "in3_storage.h"
#include "req_exec.h"
#include "transport.h"
#include "tx.h"
#include "weights.h"

#ifndef CMD_NAME
#define CMD_NAME "in3"
#endif

#define CHECK_OPTION(name, fn) \
  if (strcmp(key, name) == 0) return fn;
#ifndef IN3_VERSION
#define IN3_VERSION "local"
#endif

#define __OPTION_VALUE_MAX_SIZE 10240

static bool set_chainId(char* value, sb_t* conf) {
  if (strstr(value, "://") == NULL) return false;
  sb_add_chars(conf, "{\"rpc\":\"");
  sb_add_escaped_chars(conf, value, -1);
  sb_add_chars(conf, "\"}");
  return false;
}

bool show_help() {
  recorder_print(0, "Usage: " CMD_NAME " <options> method <params> ... \n\n%s", get_help_args());
  recorder_exit(0);
  return true;
}
bool show_version() {
  recorder_print(0, "in3 " IN3_VERSION "\nbuild " __DATE__ " with");
#ifdef TEST
  recorder_print(0, " -DTEST=true");
#endif
#ifdef EVM_GAS
  recorder_print(0, " -DEVM_GAS=true");
#endif
#ifdef CMD
  recorder_print(0, " -DCMD=true");
#endif
#ifdef IN3_MATH_FAST
  recorder_print(0, " -DFAST_MATH=true");
#endif
#ifdef IN3_SERVER
  recorder_print(0, " -DIN3_SERVER=true");
#endif
#ifdef USE_CURL
  recorder_print(0, " -DUSE_CURL=true");
#else
  recorder_print(0, " -DUSE_CURL=false");
#endif
  recorder_print(0, "\n(c) " IN3_COPYRIGHT "\n");
  recorder_exit(0);
  return true;
}

#ifdef NODESELECT_DEF
static bool set_nodelist(in3_t* c, char* nodes, sb_t* sb, bool update) {
  if (!update) c->flags = FLAGS_STATS | FLAGS_BOOT_WEIGHTS | (c->flags & FLAGS_ALLOW_EXPERIMENTAL);
  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  if (!update && nl && nl->nodelist_upd8_params) {
    _free(nl->nodelist_upd8_params);
    nl->nodelist_upd8_params = NULL;
  }
  char* cpy;
  _strncpy(cpy, nodes, __OPTION_VALUE_MAX_SIZE);
  char* s = NULL;
  sb_add_chars(sb, "{\"autoUpdateList\":false,\"nodes\":{\"needsUpdate\":false,\"nodeList\":[");
  for (char* next = strtok(cpy, ","); next; next = strtok(NULL, ",")) {
    if (next != cpy) sb_add_char(sb, ',');
    str_range_t address, url;

    if (*next == '0' && next[1] == 'x' && (s = strchr(next, ':'))) {
      address = (str_range_t){.data = next, .len = s - next};
      url     = (str_range_t){.data = s + 1, .len = strlen(s + 1)};
    }
    else {
      address = (str_range_t){.data = "0x1234567890123456789012345678901234567890", .len = 42};
      url     = (str_range_t){.data = next, .len = strlen(next)};
    }
    sb_add_chars(sb, "{\"address\":\"");
    sb_add_range(sb, address.data, 0, address.len);
    sb_add_chars(sb, "\",\"url\":\"");
    sb_add_range(sb, url.data, 0, url.len);
    sb_add_chars(sb, "\",\"props\":\"0xffff\"}");
  }
  sb_add_chars(sb, "]}}}");
  return false;
}
#endif

static bool set_data(char* value) {
  if (strcmp(value, "-") == 0)
    get_txdata()->data = get_std_in();
  else if (*value == '0' && value[1] == 'x')
    get_txdata()->data = hex_to_new_bytes(value + 2, strnlen(value, __OPTION_VALUE_MAX_SIZE) - 2);
  else {
    FILE*   f          = fopen(value, "r");
    bytes_t content    = readFile(f);
    get_txdata()->data = hex_to_new_bytes((char*) content.data + 2, content.len - 2);
    fclose(f);
  }
  return true;
}
static bool set_string(char** dst, char* value) {
  *dst = value;
  return true;
}
static bool set_uint64(uint64_t* dst, char* value) {
  // TODO support gwei or hex
  *dst = (uint64_t) atoll(value);
  return true;
}
static bool set_uint32(uint32_t* dst, char* value) {
  // TODO support gwei or hex
  *dst = (uint32_t) atoi(value);
  return true;
}
static bool set_create2(char* value, sb_t* sb) {
  if (strnlen(value, __OPTION_VALUE_MAX_SIZE) != 176) die("create2-arguments must have the form -zc2 <creator>:<codehash>:<saltarg>");
  char tmp[177];
  memcpy(tmp, value, 177);
  tmp[42] = tmp[109] = 0;
  sb_printx(sb, "{\"zksync\":{\"signer_type\":\"create2\",\"create2\":{\"creator\":\"%s\",\"codehash\":\"%s\",\"saltarg\":\"%s\"}}}", tmp, tmp + 43, tmp + 110);
  return false;
}
static bool set_recorder(in3_t* c, char* value, int argc, char** argv, bool write) {
  if (write)
    recorder_write_start(c, value, argc, argv);
  else
    recorder_read_start(c, value);
  return true;
}
static bool set_pk(in3_t* c, char* value, int argc, char** argv, in3_curve_type_t type) {
  if (strchr(value, ' ')) {
    char* seedphrase = value;
    char* passwd     = get_argument(argc, argv, "-pwd", "--password", true);
    char* path       = get_argument(argc, argv, "-path", "--path", true);
    char* err        = eth_set_pk_signer_from_string(c, seedphrase, path, passwd);
    if (err) die(err);
    return true;
  }
  else if (value[0] != '0' || value[1] != 'x') {
    read_pk(value, get_argument(argc, argv, "-pwd", "--password", true), c, NULL, type);
    return true;
  }
  else
    return false;
}
static bool set_flag(uint32_t* dst, uint32_t val, char* value) {
  if (strcmp(value, "true") == 0)
    *dst |= val;
  else
    *dst ^= val;
  return true;
}
static bool set_quiet() {
  in3_log_set_level(LOG_FATAL);
  return true;
}
static bool set_debug() {
  in3_log_set_quiet(false);
  in3_log_set_level(LOG_TRACE);
  *get_output_conf() |= out_debug;
  return true;
}

#ifdef LEDGER_NANO
static bool set_path(in3_t* c, char* value) {
  if (value[0] == '0' && value[1] == 'x') {
    bytes32_t path;
    hex_to_bytes(value, -1, path, 5);
    eth_ledger_set_signer_txn(c, path);
    return true;
  }
  else
    die("Invalid path for nano ledger");
}
#endif
bool handle_option(in3_t* c, char* key, char* value, sb_t* conf, int argc, char** argv) {
  CHECK_OPTION("test", set_test_transport(c, value))
  CHECK_OPTION("clearCache", true)
  CHECK_OPTION("password", true)
  CHECK_OPTION("help", show_help())
  CHECK_OPTION("version", show_version())
  CHECK_OPTION("chainId", set_chainId(value, conf))
  CHECK_OPTION("from", set_string(&get_txdata()->from, value))
  CHECK_OPTION("token", set_string(&get_txdata()->token, value))
  CHECK_OPTION("to", set_string(&get_txdata()->to, value))
  CHECK_OPTION("gas", set_uint64(&get_txdata()->gas, value))
  CHECK_OPTION("gas_price", set_uint64(&get_txdata()->gas_price, value))
  CHECK_OPTION("nonce", set_uint64(&get_txdata()->nonce, value))
  CHECK_OPTION("wait", set_uint32(&get_txdata()->wait, "1"))
  CHECK_OPTION("block", set_string(&get_txdata()->block, value))
  CHECK_OPTION("data", set_data(value))
  CHECK_OPTION("value", set_string(&get_txdata()->value, get_wei(value)))
  CHECK_OPTION("zksync.create2", set_create2(value, conf))
  CHECK_OPTION("test-request", set_flag(get_weightsdata(), weight_test_request, value))
  CHECK_OPTION("test-health-request", set_flag(get_weightsdata(), weight_health, value))
  CHECK_OPTION("response.in", set_response_file(true))
  CHECK_OPTION("response.out", set_response_file(false))
  CHECK_OPTION("file.in", set_recorder(c, value, argc, argv, false))
  CHECK_OPTION("file.out", set_recorder(c, value, argc, argv, true))
  CHECK_OPTION("ms.signatures", true)
  CHECK_OPTION("human", set_flag(get_output_conf(), out_human, value))
  CHECK_OPTION("eth", set_flag(get_output_conf(), out_eth, value))
  CHECK_OPTION("hex", set_flag(get_output_conf(), out_hex, value))
  CHECK_OPTION("json", set_flag(get_output_conf(), out_json, value))
  CHECK_OPTION("quiet", set_quiet())
  CHECK_OPTION("port", set_string(&get_req_exec()->port, value))
  CHECK_OPTION("allowed-methods", set_string(&get_req_exec()->allowed_methods, value))
  CHECK_OPTION("onlysign", set_onlyshow_rawtx())
  CHECK_OPTION("sigtype", set_string(&get_txdata()->signtype, value))
  CHECK_OPTION("debug", set_debug())
#ifdef NODESELECT_DEF
  CHECK_OPTION("nodelist", set_nodelist(c, value, conf, false))
  CHECK_OPTION("bootnodes", set_nodelist(c, value, conf, true))
  CHECK_OPTION("pk", set_pk(c, value, argc, argv, ECDSA_SECP256K1))
  CHECK_OPTION("pk_ed25519", set_pk(c, value, argc, argv, EDDSA_ED25519))
#endif
#ifdef LEDGER_NANO
  CHECK_OPTION("path", set_path(c, valuev))
#endif
  return false;
}

void init_recorder(int* argc, char*** argv) {
  char* file = get_argument(*argc, *argv, "-fi", "--file.in", true);
  if (file)
    recorder_update_cmd(file, argc, argv);
}

void init_env(in3_t* c, int argc, char* argv[]) {
  // handle clear cache opt before initializing cache
  if (get_argument(argc, argv, "-ccache", "--clearCache", false))
    storage_clear(NULL);
  // use the storagehandler to cache data in .in3
  in3_register_file_storage(c);

  // PK
  if (getenv("IN3_PK") && !get_argument(argc, argv, "-pk", "--pk", true)) {
    char* pks = _strdupn(getenv("IN3_PK"), -1);
    for (char* cc = strtok(pks, ","); cc; cc = strtok(NULL, ",")) eth_set_pk_signer_from_string(c, cc, NULL, NULL);
  }
  // PK
  if (getenv("IN3_PK_ED25519") && !get_argument(argc, argv, "-pk_ed25519", "--pk_ed25519", true)) {
    char*     pks = _strdupn(getenv("IN3_PK_ED25519"), -1);
    bytes32_t pk;
    for (char* cc = strtok(pks, ","); cc; cc = strtok(NULL, ",")) {
      hex_to_bytes(cc, -1, pk, 32);
      eth_set_pk_signer(c, pk, EDDSA_ED25519, NULL);
    }
  }

  // is there a config-file we can load?
  char* cnf = getenv("IN3_CONF");
  if (!cnf) {
    char* home = get_storage_dir();
    cnf        = alloca(strlen(home) + 20);
    strcpy(cnf, home);
    strcpy(cnf + strlen(cnf) - 1, "_conf.json");
  }

  if (cnf && strcmp(cnf, "none")) {
    FILE* cnf_file = fopen(cnf, "r");
    if (cnf_file) {
      char* data  = (char*) readFile(cnf_file).data;
      char* error = in3_configure(c, data);
      recorder_configure(data);

      if (error)
        die(sprintx("Error reading the in3_conf.json : %s", error));
      else
        fclose(cnf_file);
    }
    else if (getenv("IN3_CONF"))
      die(sprintx("Could not find the config file in %s", cnf));
  }

  // handle chainId
  if (getenv("IN3_CHAIN")) configure(c, "chainId", getenv("IN3_CHAIN"));

#ifdef ZKSYNC
  if (getenv("IN3_ZKS")) configure(c, "zksync.provider_url", getenv("IN3_ZKS"));
#endif
}