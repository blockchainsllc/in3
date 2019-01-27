#ifndef TEST
#define TEST
#endif
#include "vm_runner.h"
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

void print_error(char* msg) {
  if (use_color)
    printf("\x1B[31m%s\x1B[0m", msg);
  else
    printf("!! %s", msg);
}
void print_success(char* msg) {
  if (use_color)
    printf("\x1B[32m%s\x1B[0m", msg);
  else
    printf(".. %s", msg);
}

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

int run_test(d_token_t* test, int counter, char* name, uint32_t props) {
  char  temp[300];
  char* descr;
  int   i;

  int l = strlen(name), fail = 0;
  if (name[l - 5] == '.') name[l - 5] = 0;
  char*    tname = d_get_keystr(test->key);
  uint64_t ms    = 0;

  // debug
  //  if (strcmp(tname, "dup1") == 0) props |= EVM_PROP_DEBUG;

  if (tname)
    sprintf(temp, "%s : %s", name, tname);
  else
    sprintf(temp, "%s #%i", name, counter);
  printf("\n%2i : %-80s ", counter, temp);
  fflush(stdout);

  d_token_t* exec        = d_get(test, key("exec"));
  d_token_t* transaction = d_get(test, key("transaction"));

  if (d_len(test) == 2 && d_get(test, key("in")) && d_get(test, key("out")))
    fail = test_rlp(test, props, &ms);
  else if (d_get(test, key("root")) && d_get(test, key("in")))
    fail = test_trie(test, props | (strstr(name, "secure") ? 2 : 0), &ms);
  else if (exec || transaction)
    fail = test_evm(test, props, &ms);
  else {
    fail = -1;
    print_error("Unknown TestType!");
  }

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
      props |= EVM_PROP_DEBUG;
    else if (strcmp(argv[i], "-c") == 0)
      props |= EVM_PROP_CONSTANTINOPL;
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