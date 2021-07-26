/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

/** @file
 * simple commandline-util sending in3-requests.
 * */
#include "helper.h"
#include "../../api/eth1/abi.h"
#include "../../api/eth1/eth_api.h"
#include "../../signer/pk-signer/signer.h"
#include "../../tools/recorder/recorder.h"
#ifdef CMD_ARGS_FILE
#include CMD_ARGS_FILE
#else
#include "args.h"
#endif
#include "handlers.h"
#include "transport.h"
#include "tx.h"

#ifndef IN3_VERSION
#define IN3_VERSION "local"
#endif

void die(char* msg) {
  recorder_print(1, COLORT_RED "Error: %s" COLORT_RESET "\n", msg);
  recorder_exit(EXIT_FAILURE);
}
void print_hex(uint8_t* data, int len) {
  recorder_print(0, "0x");
  for (int i = 0; i < len; i++) recorder_print(0, "%02x", data[i]);
  recorder_print(0, "\n");
}

const char* get_help_args() {
  return help_args;
}

// helper to read the password from tty
void read_pass(char* pw, int pwsize) {
  int i = 0, ch = 0;
  recorder_print(1, COLORT_HIDDEN); // conceal typing and save position
  while (true) {
    ch = getchar();
    if (ch == '\r' || ch == '\n' || ch == EOF) break; // get characters until CR or NL
    if (i < pwsize - 1) {                             // do not save pw longer than space in pw
      pw[i]     = ch;                                 // longer pw can be entered but excess is ignored
      pw[i + 1] = 0;
    }
    i++;
  }
  recorder_print(1, COLORT_RESETHIDDEN); // reveal typing
}

static bool is_number(char* val) {
  for (; *val; val++) {
    if (*val > '9' || *val < '0') return false;
  }
  return true;
}

void configure_opt(in3_t* c, char* name, char* value, int argc, char** argv) {
  sb_t sb = {0};
  // handle options
  if (handle_option(c, name, value, &sb, argc, argv)) return;
  if (!sb.data) {
    char* _name = alloca(strlen(name + 1));
    strcpy(_name, name);
    char* p = strtok(_name, ".");
    sb_add_char(&sb, '{');
    int b = 1;
    while (p) {
      char* next = strtok(NULL, ".");
      if (!next) {
        if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0 || is_number(value) || *value == '{' || *value == '[')
          sb_print(&sb, "\"%s\":%s", p, value);
        else
          sb_print(&sb, "\"%s\":\"%s\"", p, value);
        break;
      }
      b++;
      sb_print(&sb, "\"%s\":{", p);
      p = next;
      continue;
    }
    for (; b; b--) sb_add_char(&sb, '}');
  }
  char* error = in3_configure(c, sb.data);
  _free(sb.data);
  if (error) {
    char* msg = _malloc(200 + (strlen(error) + strlen(name) + strlen(value)));
    sprintf(msg, "Invalid option '--%s=%s' : %s", name, value, error);
    die(msg);
  }
}
void configure(in3_t* c, char* name, char* value) {
  configure_opt(c, name, value, 0, NULL);
}
static bool is_bool_opt(char* name) {
  for (int i = 0; bool_props[i]; i++) {
    if (strcmp(bool_props[i], name) == 0) return true;
  }
  return false;
}
bool configure_arg(in3_t* c, char** args, int* index, int argc) {
  const char* arg   = args[*index];
  char*       value = strchr(arg, '=');
  char*       name  = NULL;
  if (arg[0] != '-') return false;
  if (arg[1] && arg[1] != '-') {
    for (int i = 0; aliases[i]; i += 2) {
      if (strcmp(aliases[i], arg + 1) == 0) {
        name = alloca(strlen(aliases[i + 1]) + 1);
        strcpy(name, aliases[i + 1]);
        value = strchr(aliases[i + 1], '=');
        if (value) {
          *strchr(name, '=') = 0;
          value++;
        }
        break;
      }
    }
    if (!name) {
      char* err = alloca(strlen(arg) + 200);
      sprintf(err, "Unknown option '%s'!", arg);
      die(err);
    }
  }
  else if (arg[1] != '-')
    return false;

  if (!name) {
    if (value) {
      value++;
      name = alloca(value - arg - 2);
      strncpy(name, arg + 2, value - arg - 3);
      name[value - arg - 1] = 0;
    }
    else {
      name = alloca(strlen(arg) - 1);
      strcpy(name, arg + 2);
    }
  }

  if (!value) {
    if (is_bool_opt(name))
      value = "true";
    else {
      if (argc - 1 <= *index) die("missing value for option");
      *index += 1;
      value = args[*index];
    }
  }

  configure_opt(c, name, value, argc, args);
  return true;
}

// accepts a value as
// 0.1eth
// 2keth
// 2.3meth
char* get_wei(char* val) {
  if (*val == '0' && val[1] == 'x') return val;
  bytes32_t tmp;
  int       s = string_val_to_bytes(val, NULL, tmp);
  if (s < 0) die("Invalid numeric value");
  char* res = _malloc(s * 2 + 3);
  bytes_to_hex(tmp, s, res + 2);
  if (res[2] == '0') res++;
  res[0] = '0';
  res[1] = 'x';
  return res;
}

char* resolve(in3_t* c, char* name) {
  if (!name) return NULL;
  if (name[0] == '0' && name[1] == 'x') return name;
  if (strstr(name, ".eth")) {
    char* params = alloca(strlen(name) + 10);
    sprintf(params, "[\"%s\"]", name);
    char *res = NULL, *err = NULL;
    in3_client_rpc(c, "in3_ens", params, &res, &err);
    if (err) {
      res = alloca(strlen(err) + 100);
      sprintf(res, "Could not resolve %s : %s", name, err);
      die(res);
    }
    if (res[0] == '"') {
      res[strlen(res) - 1] = 0;
      res++;
    }
    return res;
  }
  return name;
}

// read data from a file and return the bytes
bytes_t readFile(FILE* f) {
  if (!f) die("Could not read the input file");
  size_t   allocated = 1024, len = 0, r;
  uint8_t* buffer = _malloc(1025);
  while (1) {
    r = fread(buffer + len, 1, allocated - len, f);
    len += r;
    if (feof(f)) break;
    size_t new_alloc = allocated * 2 + 1;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }
  buffer[len] = 0;
  return bytes(buffer, len);
}

// read from stdin and try to optimize hexdata.
bytes_t* get_std_in() {
  if (feof(stdin)) return NULL;
  bytes_t content = readFile(stdin);

  // this check tries to discover a solidity compilation poutput
  char* bin = strstr((char*) content.data, "\nBinary: \n");
  if (bin) {
    bin += 10;
    char* end = strstr(bin, "\n");
    if (end)
      return hex_to_new_bytes(bin, end - bin);
  }

  // is it content starting with 0x, we treat it as hex otherwisae as rwa string
  bytes_t* res = (content.len > 1 && *content.data == '0' && content.data[1] == 'x')
                     ? hex_to_new_bytes((char*) content.data + 2, content.len - 2)
                     : hex_to_new_bytes((char*) content.data, content.len);
  _free(content.data);
  return res;
}

void print_val(d_token_t* t) {
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT: {
      char* level = d_get_string(t, key("level"));
      if (level) {
        char* msg = d_get_string(t, key("msg"));
        if (strcmp(level, "main") == 0) recorder_print(0, COLOR_GREEN_STR "\n", msg);
        if (strcmp(level, "info") == 0) recorder_print(0, "%s\n", msg);
        if (strcmp(level, "warning") == 0) recorder_print(0, COLOR_YELLOW_STR "\n", msg);
        if (strcmp(level, "error") == 0) recorder_print(0, COLOR_RED_STR "\n", msg);
      }
      else {
        for (d_iterator_t it = d_iter(t); it.left; d_iter_next(&it))
          print_val(it.token);
      }
    } break;
    case T_BOOLEAN:
      recorder_print(0, "%s\n", d_int(t) ? "true" : "false");
      break;
    case T_INTEGER:
      recorder_print(0, "%i\n", d_int(t));
      break;
    case T_BYTES:
      if (t->len < 9)
        recorder_print(0, "%" PRId64 "\n", d_long(t));
      else {
        recorder_print(0, "0x");
        for (int i = 0; i < (int) t->len; i++) recorder_print(0, "%02x", t->data[i]);
        recorder_print(0, "\n");
      }
      break;
    case T_NULL:
      recorder_print(0, "NULL\n");
      break;
    case T_STRING:
      recorder_print(0, "%s\n", d_string(t));
      break;
  }
}
// decode pk
void read_pk(char* pk_file, char* pwd, in3_t* c, char* method) {
  if (pk_file) {
    if (!pwd) {
      recorder_print(1, "Passphrase:\n");
      pwd = malloc(500);
      read_pass(pwd, 500);
    }
    char* content;
    if (strcmp(pk_file, "-") == 0)
      content = (char*) readFile(stdin).data;
    else if (pk_file[0] == '{')
      content = pk_file;
    else
      content = (char*) readFile(fopen(pk_file, "r")).data;

    json_ctx_t* key_json = parse_json(content);
    if (!key_json) die("invalid json in pk file");

    uint8_t* pk_seed = malloc(32);
    if (decrypt_key(key_json->result, pwd, pk_seed)) die("Invalid key");

    if (!c && method && (strcmp(method, "keystore") == 0 || strcmp(method, "key") == 0)) {
      char tmp[64];
      bytes_to_hex(pk_seed, 32, tmp);
      recorder_print(0, "0x%s\n", tmp);
      recorder_exit(0);
    }
    else
      eth_set_pk_signer(c, pk_seed);
  }
}

char* get_argument(int argc, char* argv[], char* alias, char* arg, bool has_value) {
  int l = strlen(arg);
  for (int i = 1; i < argc; i++) {
    if (alias && strcmp(alias, argv[i]) == 0)
      return has_value ? (i + 1 < argc ? argv[i + 1] : NULL) : argv[i];
    if (strncmp(arg, argv[i], l) == 0) {
      if (argv[i][l] == 0)
        return has_value ? (i + 1 < argc ? argv[i + 1] : NULL) : argv[i];
      else if (argv[i][l] == '=')
        return argv[i] + l + 1;
    }
  }
  return NULL;
}

static uint32_t conf = 0;
uint32_t*       get_output_conf() {
  return &conf;
}

void display_result(char* method, char* result) {
  // if the result is a string, we remove the quotes
  if ((conf & out_human) == 0 && result[0] == '"' && result[strlen(result) - 1] == '"') {
    memmove(result, result + 1, strlen(result));
    result[strlen(result) - 1] = 0;
  }

  abi_sig_t* req = get_txdata()->abi_sig;

  // if the request was a eth_call, we decode the result
  if (req) {
    int l = strlen(result) / 2 - 1;
    if (l) {
      char*       error = NULL;
      uint8_t*    tmp   = alloca(l + 1);
      json_ctx_t* res   = abi_decode(req, bytes(tmp, hex_to_bytes(result, -1, tmp, l + 1)), &error);
      if (error) die(error);
      if (conf & out_json)
        recorder_print(0, "%s\n", d_create_json(res, res->result));
      else
        print_val(res->result);
    }
    // if not we simply print the result
  }
  else if (conf & out_human) {
    json_ctx_t* jctx = parse_json(result);
    if (jctx)
      print_val(jctx->result);
    else
      recorder_print(0, "%s\n", result);
  }
  else if (is_onlyshow_rawtx() && strcmp(method, "in3_prepareTx") == 0 && get_txdata()->from)
    recorder_print(0, "%s %s\n", result, get_txdata()->from);
  else {
    if (conf & out_eth && result[0] == '0' && result[1] == 'x' && strlen(result) <= 18) {
      double val = char_to_long(result, strlen(result));
      recorder_print(0, "%.3f\n", val / 1000000000000000000L);
    }
    else if ((conf & out_hex) == 0 && result[0] == '0' && result[1] == 'x' && strlen(result) <= 18)
      recorder_print(0, "%" PRIu64 "\n", char_to_long(result, strlen(result)));
    else
      recorder_print(0, "%s\n", result);
  }
}

static inline bool is_convertable_number(char* c) {
  if (!*c) return false;
  for (; *c; c++) {
    if ((*c < '0' || *c > '9') && *c != '.') return strcmp(c, "eth") == 0;
  }
  return true;
}

static inline bool is_json(char* c) {
  char* e = parse_json_error(c);
  if (e) {
    _free(e);
    return false;
  }
  return true;
}
static inline bool needs_hex_number(char* arg, char* method) {
  // for eth, zksync (but not for eth_feeHistory)
  return (strncmp(method, "eth_", 4) == 0 || strncmp(method, "zk", 2) == 0) && strcmp(method, "eth_feeHistory") && is_convertable_number(arg);
}

void add_argument(char* arg, sb_t* args, in3_t* c, char* method) {
  if (needs_hex_number(arg, method))
    sb_print(args, "\"%s\"", get_wei(arg));
  else
    sb_print(args, is_json(arg) ? "%s" : "\"%s\"", strcmp(method, "in3_ens") ? resolve(c, arg) : arg);
}