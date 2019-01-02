#ifndef TEST
#define TEST
#endif
#include <core/client/client.h>
#include <core/client/context.h>
#include <core/util/utils.h>
#include <eth_full/eth_full.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ignore_property(char* name, int full_proof) {
  // we cannot add all difiiculties from the beginning in order to verify it,
  if (strcmp(name, "totalDifficulty") == 0) return 1;
  // accounts have keys withn the address. Manipulating them makes no sense, since only the address inside is used.
  if (strcmp(name, "accounts") == 0) return 1;

  // this is not part of the standatd RPC
  if (strcmp(name, "transactionLogIndex") == 0) return 1;

  // size should be verified if proof = full
  if (!full_proof && strcmp(name, "size") == 0) return 1;

  // gasUsed should be verified if proof = full
  if (!full_proof && strcmp(name, "gasUsed") == 0) return 1;

  return 0;
}

static int use_color = 0;

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

static char*      _tmp_str;
static d_token_t* _tmp_response;
static int        fuzz_pos = -1;

static int find_hex(char* str, int start, int len) {
  int i;
  for (i = start, str += start; i < len; i++, str++) {
    if (*str == 'x' && i > 2 && *(str - 1) == '0' && i < len + 1 && *(str + 1) != '"')
      return i + 1;
  }
  return -1;
}
static void mod_hex(char* c) {
  if (*c == 'f' || *c == 'F')
    *c = 'e';
  else if (*c == '9')
    *c = 'd';
  else
    *c = *c + 1;
}

static str_range_t find_prop_name(char* p, char* start) {
  int         i = 0, offset = start - p;
  int         prop = 0, end = 0;
  str_range_t res;
  for (i = 0; i > offset; i--) {
    if (!prop) {
      if (p[i] == ':') prop = 1;
    } else if (p[i] == '"') {
      if (!end)
        end = i;
      else {
        res.data = p + i + 1;
        res.len  = end - i - 1;
        return res;
      }
    }
  }
  res.data = NULL;
  return res;
}

static int send_mock(char** urls, int urls_len, char* payload, in3_response_t* result) {
  // printf("payload: %s\n",payload);
  int i;
  for (i = 0; i < urls_len; i++) {
    str_range_t r = d_to_json(d_get_at(_tmp_response, i));
    sb_add_char(&(result + i)->result, '[');
    sb_add_range(&(result + i)->result, r.data, 0, r.len);
    sb_add_char(&(result + i)->result, ']');

    if (fuzz_pos >= 0 && i == 0)
      mod_hex((result + i)->result.data + fuzz_pos + 1);
  }
  return 0;
}

int execRequest(in3_t* c, d_token_t* test, int must_fail) {
  d_token_t* request  = d_get(test, key("request"));
  d_token_t* response = d_get(test, key("response"));
  d_token_t* config   = d_get(request, key("config"));
  d_token_t* t;
  char*      method;
  char       params[10000];

  // configure in3
  c->requestCount = (t = d_get(config, key("requestCount"))) ? d_int(t) : 1;
  method          = d_get_string(request, "method");

  str_range_t s = d_to_json(d_get(request, key("params")));
  if (!method) {
    printf("NO METHOD");
    return -1;
  }
  if (!s.data) {
    printf("NO PARAMS");
    return -1;
  }
  strncpy(params, s.data, s.len);
  params[s.len] = 0;

  char *res, *err;
  int   success = must_fail ? 0 : d_get_intkd(test, key("success"), 1);

  _tmp_response = response;

  in3_client_rpc(c, method, params, &res, &err);

  if (err && res) {
    print_error("Error and Result set");
    _free(err);
    _free(res);
    return -1;

  } else if (err) {
    if (success) {
      print_error("Failed");
      printf(":%s", err);
      _free(err);
      return -1;
    }
    /*
        if ((t=ctx_get_token(str,test,"error")) && strncmp(str+t->start, err, t->end - t->start)!=0) {
                printf("wrong error: %s", err);
               _free(err);
               return -1;
        }
        */
    print_success("OK");
    _free(err);
    return 0;
  } else if (res) {
    if (!success) {
      print_error("Should have Failed");
      _free(res);
      return -1;
    }
    print_success("OK");
    _free(res);
    return 0;
  } else {
    print_error("NO Error and no Result");
    return -1;
  }
}

int run_test(d_token_t* test, int counter, char* fuzz_prop, in3_proof_t proof) {
  char  temp[300];
  char* descr;
  int   i;

  if ((descr = d_get_string(test, "descr"))) {
    if (fuzz_prop)
      sprintf(temp, "  ...  manipulate #%s", fuzz_prop);
    else
      strcpy(temp, descr);
  } else
    sprintf(temp, "Request #%i", counter);
  printf("\n%2i : %-60s ", counter, temp);

  in3_t* c = in3_new();
  int    j;
  c->max_attempts = 1;
  c->transport    = send_mock;
  for (j = 0; j < c->chainsCount; j++)
    c->chains[j].needsUpdate = false;
  c->proof = proof;

  d_token_t* signatures = d_get(test, key("signatures"));
  c->chainId            = d_get_longkd(test, key("chainId"), 1);
  if (signatures) {
    c->signatureCount = d_len(signatures);
    for (j = 0; j < c->chainsCount; j++) {
      if (c->chains[j].chainId == c->chainId) {
        for (i = 0; i < c->chains[j].nodeListLength; i++) {
          if (i < c->signatureCount)
            memcpy(c->chains[j].nodeList[i].address->data, d_get_bytes_at(signatures, i)->data, 20);
          else
            c->chains[j].weights[i].blacklistedUntil = 0xFFFFFFFFFFFFFF;
        }
      }
    }
  }

  int fail = execRequest(c, test, fuzz_prop != NULL);

  in3_free(c);

  if (mem_get_memleak_cnt()) {
    printf(" -- Memory Leak detected by malloc #%i!", mem_get_memleak_cnt());
    if (!fail) fail = 1;
  }

  size_t           max_heap = mem_get_max_heap();
  str_range_t      res_size = d_to_json(_tmp_response);
  bytes_builder_t* bb       = bb_new();

  d_serialize_binary(bb, _tmp_response);

  _tmp_response = NULL;
  _tmp_str      = NULL;

  printf(" ( heap: %zu json: %lu bin: %i) ", max_heap, res_size.len, bb->b.len);
  bb_free(bb);
  return fail;
}

int runRequests(char* name, int test_index, int mem_track) {
  int   res     = 0;
  char* content = readContent(name);
  char  tmp[300];
  if (content == NULL)
    return -1;

  // create client

  // TODO init the nodelist
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

  int failed = 0, total = 0, count = 0;

  if ((tests = parsed->items)) {
    for (i = 0, test = tests + 1; i < d_len(tests); i++, test = d_next(test)) {

      fuzz_pos          = -1;
      in3_proof_t proof = PROOF_STANDARD;
      if ((str_proof = d_get_string(test, "proof"))) {
        if (strcmp(str_proof, "none") == 0) proof = PROOF_NONE;
        if (strcmp(str_proof, "standard") == 0) proof = PROOF_STANDARD;
        if (strcmp(str_proof, "full") == 0) proof = PROOF_FULL;
      }

      count++;
      if (test_index < 0 || count == test_index) {
        total++;
        mem_reset(mem_track);
        if (run_test(test, count, NULL, proof)) failed++;
      }

      if (d_get_int(test, "fuzzer")) {
        str_range_t resp = d_to_json(d_get_at(d_get(test, key("response")), 0));
        while ((fuzz_pos = find_hex(resp.data, fuzz_pos + 1, resp.len)) > 0) {
          str_range_t prop = find_prop_name(resp.data + fuzz_pos, resp.data);
          if (prop.data == NULL) continue;
          strncpy(tmp, prop.data, prop.len);
          tmp[prop.len] = 0;

          if (ignore_property(tmp, proof == PROOF_FULL)) continue;

          count++;
          if (test_index > 0 && count != test_index) continue;
          total++;
          mem_reset(mem_track);
          if (run_test(test, count, tmp, proof)) failed++;
        }
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

  printf("\n%2i of %2i successfully tested", total - failed, total);

  if (failed) {
    printf("\n%2i tests failed", failed);
    res = failed;
  }
  printf("\n");

  return failed;
}

int main(int argc, char* argv[]) {
  use_color = 1;
  in3_register_eth_full();
  return runRequests(argv[1], argc > 2 ? atoi(argv[2]) : -1, argc > 3 ? atoi(argv[3]) : -1);
}