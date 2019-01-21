#ifndef TEST
#define TEST
#endif
#include <core/client/client.h>
#include <core/client/context.h>
#include <core/client/keys.h>
#include <core/util/data.h>
#include <core/util/utils.h>
#include <eth_full/big.h>
#include <eth_full/evm.h>
#include <eth_nano/rlp.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int use_color = 1;

static void print_error(char* msg) {
  if (use_color)
    printf("\x1B[31m%s\x1B[0m", msg);
  else
    printf("!! %s", msg);
}
static void print_success(char* msg) {
  if (use_color)
    printf("\x1B[32m%s\x1B[0m", msg);
  else
    printf(".. %s", msg);
}

#define ERROR(s) printf("Error: %s", s)

char* readContent(char* name) {
  char temp[500];
  sprintf(temp, strchr(name, '.') == NULL ? "../test/testdata/%s.json" : "%s", name);
  FILE* file = fopen(temp, "r");
  if (file == NULL) {
    ERROR("could not open the file");
    return NULL;
  }

  size_t allocated = 1024, len = 0, r = 0;
  char*  buffer = malloc(1024);
  while (1) {
    r = fread(buffer + len, 1, allocated - len - 1, file);
    len += r;
    if (feof(file)) break;
    buffer = realloc(buffer, allocated *= 2);
  }
  buffer[len] = 0;

  if (file)
    fclose(file);

  return buffer;
}
static bytes_t current_block = {.data = NULL, .len = 0};
d_token_t*     vm_get_account(d_token_t* test, uint8_t* adr) {
  char tmp[44];
  tmp[0] = '0';
  tmp[1] = 'x';
  int8_to_char(adr, 20, tmp + 2);
  return d_get(d_get(test, key("pre")), key(tmp));
}
d_token_t* vm_get_storage(d_token_t* test, uint8_t* adr, uint8_t* k, int l_key) {
  char tmp[68];
  tmp[0] = '0';
  tmp[1] = 'x';
  int8_to_char(k, l_key, tmp + 2);
  return d_get(d_get(vm_get_account(test, adr), key("storage")), key(tmp));
}
static uint8_t __zero = 0, __tmp[32];

int runner_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len) {
  bytes_t res;

  d_token_t *t, *t2;
  int        i;

  evm_t* evm = evm_ptr;
  if (!evm) return EVM_ERROR_INVALID_ENV;
  d_token_t* test = evm->env_ptr;
  if (!test) return EVM_ERROR_INVALID_ENV;

  switch (evm_key) {
    case EVM_ENV_BLOCKHEADER:
      *out_data = current_block.data;
      return current_block.len;

    case EVM_ENV_BALANCE:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_BALANCE));
        if (res.data != NULL) {
          *out_data = res.data;
          return res.len;
        }
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_STORAGE:
      if ((t = vm_get_storage(test, evm->address, in_data, in_len))) {
        res       = d_to_bytes(t);
        *out_data = res.data;
        return res.len;
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_BLOCKHASH:
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

    case EVM_ENV_CODE_SIZE:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          int_to_bytes(res.len, __tmp);
          *out_data = __tmp;
          return 4;
        }
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_CODE_HASH:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          sha3_to(&res, __tmp);
          *out_data = __tmp;
          return 32;
        }
      }
      *out_data = &__zero;
      return 0;

    case EVM_ENV_CODE_COPY:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          *out_data = res.data;
          return res.len;
        }
      }
      *out_data = &__zero;
      return 0;
  }
  return -2;
}
void prepare_header(d_token_t* block) {
  bytes_builder_t* rlp = bb_new();
  d_token_t *      sealed_fields, *t;
  uint8_t          data[32];
  bytes_t          tmp = {.data = data, .len = 32}, r;
  memset(data, 0, 32);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentCoinbase")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentDifficulty")));
  rlp_encode_item(rlp, &r);
  r = d_to_bytes(d_get(block, key("currentNumber")));
  rlp_encode_item(rlp, &r);
  r = d_to_bytes(d_get(block, key("currentGasLimit")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentTimestamp")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);

  // clang-format on
  bytes_t* b         = bb_move_to_bytes(rlp_encode_to_list(rlp));
  current_block.data = b->data;
  current_block.len  = b->len;
  _free(b);
}

int check_post_state(evm_t* evm, d_token_t* post) {
#ifdef EVM_GAS
  int        i, j;
  d_token_t *t, *storages, *s;
  for (i = 0, t = post + 1; i < d_len(post); i++, t = d_next(t)) {
    char*   adr_str = d_get_keystr(t->key);
    uint8_t address[20];
    hex2byte_arr(adr_str + 2, strlen(adr_str) - 2, address, 20);
    storages = d_get(t, key("storage"));
    for (j = 0, s = storages + 1; j < d_len(storages); j++, t = d_next(s)) {
      char*      s_str = d_get_keystr(s->key);
      uint8_t    s_key[32];
      int        l_key    = hex2byte_arr(s_str + 2, strlen(s_str) - 2, s_key, 32);
      bytes_t    val_must = d_to_bytes(s);
      storage_t* st       = evm_get_storage(evm, address, s_key, l_key, 0);
      if (!st) {
        print_error("Missing the storage key!");
        return -1;
      }
      if (big_cmp(st->value, 32, val_must.data, val_must.len)) {
        print_error("Wrong storage value!");
        printf("expected : ");
        b_print(&val_must);
        val_must.data = st->value;
        val_must.len  = 32;
        printf("is : ");
        b_print(&val_must);
        return -1;
      }
    }
  }
#endif
  return 0;
}

int run_test(d_token_t* test, int counter, char* name, uint32_t props) {
  char  temp[300];
  char* descr;
  int   i;

  d_token_t* exec = d_get(test, key("exec"));
  int        l    = strlen(name);
  if (name[l - 5] == '.') name[l - 5] = 0;
  char*    tname = d_get_keystr(test->key);
  uint64_t start = clock();

  // debug
  if (strcmp(tname, "dup1") == 0) props |= EVM_DEBUG;

  if (tname)
    sprintf(temp, "%s : %s", name, tname);
  else
    sprintf(temp, "%s #%i", name, counter);
  printf("\n%2i : %-80s ", counter, temp);
  fflush(stdout);

  // create vm
  evm_t evm;
  evm.stack.b.data = _malloc(64);
  evm.stack.b.len  = 0;
  evm.stack.bsize  = 64;

  evm.memory.b.data = _calloc(32, 1);
  evm.memory.b.len  = 0;
  evm.memory.bsize  = 32;

  evm.invalid_jumpdest = NULL;

  evm.stack_size = 0;

  evm.pos   = 0;
  evm.state = EVM_STATE_INIT;

  evm.last_returned.data = NULL;
  evm.last_returned.len  = 0;

  evm.properties = props; //EVM_EIP_CONSTANTINOPL;

  evm.env     = runner_get_env;
  evm.env_ptr = test;

  evm.gas_price  = d_to_bytes(d_get(exec, key("gasPrice")));
  evm.call_data  = d_to_bytes(d_get(exec, key("data")));
  evm.call_value = d_to_bytes(d_get(exec, key("value")));

  evm.return_data.data = NULL;
  evm.return_data.len  = 0;

  evm.caller  = d_get_bytes(exec, "caller")->data;
  evm.origin  = d_get_bytes(exec, "origin")->data;
  evm.address = d_get_bytes(exec, "address")->data;
  evm.account = d_get_bytes(exec, "address")->data;
#ifdef EVM_GAS
  evm.accounts = NULL;
  evm.gas      = d_get_long(exec, "gas");
  evm.code     = d_to_bytes(d_get(exec, K_CODE));
  evm.root     = &evm;
  evm.logs     = NULL;
#endif

  prepare_header(d_get(test, key("env")));

  int      fail = evm_run(&evm);
  uint64_t ms   = (clock() - start) / 1000;

  _free(current_block.data);

  // now check results...
  if (!fail) {
    bytes_t must_out = d_to_bytes(d_get(test, key("out")));
    if (must_out.len && !b_cmp(&must_out, &evm.return_data)) {
      print_error(" wrong result");
      printf("\nshould be :");
      b_print(&must_out);
      printf("\nbut is    :");
      b_print(&evm.return_data);
      fail = 1;
    } else
      // check post state
      fail = check_post_state(&evm, d_get(test, key("post")));
#ifdef EVM_GAS
    if (!fail && d_get_long(test, "gas") != evm.gas) {
      print_error("Wrong Gas");
      printf(" (expected : %llu, but got %llu", d_get_long(test, "gas"), evm.gas);
      fail = 1;
    }
#endif
  } else {
    if (fail && !d_get(test, key("post"))) fail = 0;

    switch (fail) {
      case 0:
        break;
      case EVM_ERROR_BUFFER_TOO_SMALL:
        print_error("Memory or Buffer too small!");
        break;
      case EVM_ERROR_EMPTY_STACK:
        print_error("The Stack is empty");
        break;
      case EVM_ERROR_ILLEGAL_MEMORY_ACCESS:
        print_error("There is no Memory allocated at this position.");
        break;
      case EVM_ERROR_INVALID_ENV:
        print_error("The env could not deliver the requested value.");
        break;
      case EVM_ERROR_INVALID_JUMPDEST:
        print_error("Invalid jump destination.");
        break;
      case EVM_ERROR_INVALID_OPCODE:
        print_error("Invalid op code.");
        break;
      case EVM_ERROR_INVALID_PUSH:
        print_error("Invalid push");
        break;
      case EVM_ERROR_TIMEOUT:
        print_error("timeout running the call");
        break;
      case EVM_ERROR_UNSUPPORTED_CALL_OPCODE:
        print_error("This op code is not supported with eth_call!");
        break;
      case EVM_ERROR_OUT_OF_GAS:
        print_error("Ran out of gas.");
        break;
      default:
        printf("Unknown return-code %i", fail);
        break;
    }
  }
  evm_free(&evm);

  if (mem_get_memleak_cnt()) {
    printf(" -- Memory Leak detected by malloc #%i!", mem_get_memleak_cnt());
    if (!fail) fail = 1;
  }
  if (!fail) print_success("OK");

  printf(" ( heap: %zu, %" PRIu64 " ms) ", mem_get_max_heap(), ms);

  return fail;
}

int runRequests(char** names, int test_index, int mem_track, uint32_t props) {
  int   res = 0, n = 0;
  char* name   = names[n];
  int   failed = 0, total = 0, count = 0;
  while (name) {
    char* content = readContent(name);
    char  tmp[300];
    if (content == NULL) {
      print_error("Filename not found!\n");
      return -1;
    }
    d_track_keynames(1);
    d_clear_keynames();

    json_parsed_t* parsed = parse_json(content);
    if (!parsed) {
      free(content);
      ERROR("Error parsing the requests");
      return -1;
    }
    // parse the data;
    int        i;
    char*      str_proof;
    d_token_t *t      = NULL, *tests, *test;
    d_token_t* tokens = NULL;

    if ((tests = parsed->items)) {
      for (i = 0, test = tests + 1; i < d_len(tests); i++, test = d_next(test)) {
        count++;
        if (test_index < 0 || count == test_index) {
          total++;
          mem_reset(mem_track);
          if (run_test(test, count, name, props)) failed++;
        }
      }
    }

    free(content);
    for (i = 0; i < parsed->len; i++) {
      if (parsed->items[i].data != NULL && d_type(parsed->items + i) < 2)
        free(parsed->items[i].data);
    }
    free(parsed->items);
    free(parsed);
    name = names[++n];
  }
  printf("\n%2i of %2i successfully tested", total - failed, total);

  if (failed) {
    printf("\n%2i tests failed", failed);
    res = failed;
  }
  printf("\n");

  return failed;
}

int main(int argc, char* argv[]) {
  int    i = 0, size = 1;
  int    testIndex = -1, membrk = -1;
  char** names   = malloc(sizeof(char*));
  names[0]       = NULL;
  uint32_t props = 0;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0)
      testIndex = atoi(argv[++i]);
    else if (strcmp(argv[i], "-m") == 0)
      membrk = atoi(argv[++i]);
    else if (strcmp(argv[i], "-d") == 0)
      props |= EVM_DEBUG;
    else if (strcmp(argv[i], "-c") == 0)
      props |= EVM_EIP_CONSTANTINOPL;
    else {
      //      if (strstr(argv[i], "exp") || strstr(argv[i], "loop-mulmod")) {
      //        printf("\nskipping %s\n", argv[i]);
      //        continue;
      //      }

      char** t = malloc((size + 1) * sizeof(char*));
      memmove(t, names, size * sizeof(char*));
      free(names);
      names           = t;
      names[size - 1] = argv[i];
      names[size++]   = NULL;
    }
  }

  return runRequests(names, testIndex, membrk, props);
}