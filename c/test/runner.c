/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

#ifndef TEST
#define TEST
#endif
#include "../src/api/eth1/eth_api.h"
#include "../src/core/client/client.h"
#include "../src/core/client/context.h"
#include "../src/core/util/log.h"
#include "../src/core/util/mem.h"
#include "../src/verifier/eth1/full/eth_full.h"
#include "../src/verifier/ipfs/ipfs.h"
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
  // this is not part of the standatd RPC, its part of the nodelisz, but not used anymore
  if (strcmp(name, "chainIds") == 0) return 1;
  // the registryId is taken from the local config - no need to verify
  if (strcmp(name, "registryId") == 0) return 1;
  // it is calculated, so need to check
  if (strcmp(name, "proofHash") == 0) return 1;
  // weight is not part of the proofHash
  if (strcmp(name, "weight") == 0) return 1;

  // size should be verified if proof = full
  if (!full_proof && strcmp(name, "size") == 0) return 1;
  // size should be verified if proof = full
  if (!full_proof && strcmp(name, "logIndex") == 0) return 1;

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
    size_t new_alloc = allocated * 2;
    buffer           = realloc(buffer, new_alloc);
    allocated        = new_alloc;
  }
  buffer[len] = 0;

  if (file)
    fclose(file);

  return buffer;
}

static int        fuzz_pos       = -1;
static d_token_t* _tmp_responses = NULL;
static int        _tmp_pos       = 0;
static bool       _tmp_bin       = false;

static int find_hex(char* str, int start, int len) {
  int i;
  for (i = start, str += start; i < len; i++, str++) {
    if (*str == 'x' && i > 2 && *(str - 1) == '0' && i < len + 1 && *(str + 1) != '"') {
      char* end = strchr(str, '"');
      if (end) end++;
      while (end && (*end == ' ' || *end == '\n' || *end == '\t')) end++;
      if (end && *end == ':') continue;
      return i + 1;
    }
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

static void prepare_response(int count, d_token_t* response_array, int as_bin, int _fuzz_pos) {
  _tmp_responses = response_array;
  _tmp_bin       = as_bin;
  fuzz_pos       = _fuzz_pos;
  _tmp_pos       = 0;
}

static int send_mock(in3_request_t* req) {
  int     i;
  bytes_t response;
  if (d_len(_tmp_responses) <= _tmp_pos) {
    for (i = 0; i < req->urls_len; i++)
      sb_add_chars(&(req->results + i)->error, "Reached end of available responses!");
    return IN3_EINVAL;
  }

  if (!_tmp_bin) {
    sb_t*       sb = sb_new(NULL);
    str_range_t r  = d_to_json(d_get_at(_tmp_responses, _tmp_pos));
    sb_add_char(sb, '[');
    sb_add_range(sb, r.data, 0, r.len);
    sb_add_char(sb, ']');

    if (fuzz_pos >= 0)
      mod_hex(sb->data + fuzz_pos + 1);
    response = bytes((uint8_t*) sb->data, sb->len);
    _free(sb);
  } else {
    bytes_builder_t* bb = bb_new();
    d_serialize_binary(bb, _tmp_responses + _tmp_pos + 1);
    response = bb->b;
    _free(bb);
  }

  // printf("payload: %s\n",payload);
  for (i = 0; i < req->urls_len; i++)
    sb_add_range(&(req->results + i)->result, (char*) response.data, 0, response.len);

  _free(response.data);
  _tmp_pos++;
  return 0;
}

int execRequest(in3_t* c, d_token_t* test, int must_fail, int counter, char* descr) {
  d_token_t* request  = d_get(test, key("request"));
  d_token_t* response = d_get(test, key("response"));
  d_token_t* config   = d_get(request, key("config"));
  d_token_t* t        = NULL;
  char*      method   = NULL;
  char       params[10000];

  // configure in3
  c->request_count   = (t = d_get(config, key("requestCount"))) ? d_int(t) : 1;
  method             = d_get_string(request, "method");
  bool        intern = d_get_int(test, "intern");
  str_range_t s      = d_to_json(d_get(request, key("params")));

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

  char *res = NULL, *err = NULL;
  int   success = must_fail ? 0 : d_get_intkd(test, key("success"), 1);
  if (intern) _tmp_pos++; // if this is a intern, then the first response is the expected, while the all other come after this.

  //  _tmp_response = response;
  int is_bin = d_get_int(test, "binaryFormat");

  in3_client_rpc_raw(c, d_string(request), is_bin ? NULL : &res, &err);
  fflush(stdout);
  fflush(stderr);
  printf("\n%2i : %-60s ", counter, descr);

  if (res && intern) {
    json_ctx_t* actual_json = parse_json(res);
    d_token_t*  actual      = actual_json->result;
    d_token_t*  expected    = d_get(response + 1, key("result"));
    if (!d_eq(actual, expected)) {
      err = _malloc(strlen(res) + 200);
      sprintf(err, "wrong response: %s", res);
      _free(res);
      res = NULL;
    }
    json_free(actual_json);
  }

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
  } else if (res || is_bin) {
    if (!success) {
      print_error("Should have Failed");
      if (!is_bin) _free(res);
      return -1;
    }
    print_success("OK");
    if (!is_bin) _free(res);
    return 0;
  } else {
    print_error("NO Error and no Result");
    return -1;
  }
}

int run_test(d_token_t* test, int counter, char* fuzz_prop, in3_proof_t proof) {
  char  temp[300];
  char* descr = NULL;
  int   i;
  in3_log_set_prefix("");

  if ((descr = d_get_string(test, "descr"))) {
    if (fuzz_prop)
      sprintf(temp, "  ...  manipulate #%s", fuzz_prop);
    else
      strcpy(temp, descr);
  } else
    sprintf(temp, "Request #%i", counter);

  in3_t* c = in3_for_chain(d_get_intkd(test, key("chainId"), 1));
  int    j;
  c->max_attempts        = 1;
  c->flags               = FLAGS_STATS | FLAGS_INCLUDE_CODE | FLAGS_AUTO_UPDATE_LIST;
  c->transport           = send_mock;
  c->cache               = NULL;
  d_token_t* first_res   = d_get(d_get_at(d_get(test, key("response")), 0), key("result"));
  d_token_t* registry_id = d_type(first_res) == T_OBJECT ? d_get(first_res, key("registryId")) : NULL;
  for (j = 0; j < c->chains_length; j++) {
    _free(c->chains[j].nodelist_upd8_params);
    c->chains[j].nodelist_upd8_params = NULL;

    if (registry_id) {
      c->chains[j].version = 2;
      memcpy(c->chains[j].registry_id, d_bytesl(registry_id, 32)->data, 32);
      memcpy(c->chains[j].contract->data, d_get_byteskl(first_res, key("contract"), 20)->data, 20);
    }
  }
  c->proof = proof;

  d_token_t* signatures = d_get(test, key("signatures"));
  c->chain_id           = d_get_longkd(test, key("chainId"), 1);
  if (signatures) {
    c->signature_count = d_len(signatures);
    for (j = 0; j < c->chains_length; j++) {
      if (c->chains[j].chain_id == c->chain_id) {
        for (i = 0; i < c->chains[j].nodelist_length; i++) {
          if (i < c->signature_count)
            memcpy(c->chains[j].nodelist[i].address->data, d_get_bytes_at(signatures, i)->data, 20);
          else
            c->chains[j].weights[i].blacklisted_until = 0xFFFFFFFFFFFFFF;
        }
      }
    }
  }
  int fail = execRequest(c, test, fuzz_prop != NULL, counter, temp);
  in3_free(c);

  if (mem_get_memleak_cnt()) {
    printf(" -- Memory Leak detected by malloc #%i!", mem_get_memleak_cnt());
    if (!fail) fail = 1;
  }
  d_token_t*       response = d_get(test, key("response"));
  size_t           max_heap = mem_get_max_heap();
  str_range_t      res_size = d_to_json(response);
  bytes_builder_t* bb       = bb_new();

  d_serialize_binary(bb, response);

  printf(" ( heap: %zu json: %lu bin: %i) ", max_heap, res_size.len, bb->b.len);
  bb_free(bb);
  return fail;
}

int runRequests(char** names, int test_index, int mem_track) {
  int   res = 0, n = 0;
  char* name   = names[n];
  int   failed = 0, total = 0, count = 0;
  while (name) {
    char* content = readContent(name);
    char  tmp[300];
    if (content == NULL)
      return -1;

    // create client

    // TODO init the nodelist
    json_ctx_t* parsed = parse_json(content);
    if (!parsed) {
      free(content);
      ERROR("Error parsing the requests");
      return -1;
    }

    // parse the data;
    int        i;
    char*      str_proof = NULL;
    d_token_t *t = NULL, *tests = NULL, *test = NULL;
    d_token_t* tokens = NULL;

    if ((tests = parsed->result)) {
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
          prepare_response(1, d_get(test, key("response")), d_get_int(test, "binaryFormat"), -1);
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
            prepare_response(1, d_get(test, key("response")), d_get_int(test, "binaryFormat"), fuzz_pos);
            mem_reset(mem_track);
            if (run_test(test, count, tmp, proof)) failed++;
          }
        }
      }
    }

    free(content);
    json_free(parsed);
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
  use_color = 1;
  in3_log_set_level(LOG_INFO);
  in3_register_eth_full();
  in3_register_eth_api();
  in3_register_ipfs();

  int    i = 0, size = 1;
  int    testIndex = -1, membrk = -1;
  char** names = malloc(sizeof(char*));
  names[0]     = NULL;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0)
      testIndex = atoi(argv[++i]);
    else if (strcmp(argv[i], "-d") == 0)
      in3_log_set_level(LOG_TRACE);
    else if (strcmp(argv[i], "-m") == 0)
      membrk = atoi(argv[++i]);
    else {
      char** t = malloc((size + 1) * sizeof(char*));
      memmove(t, names, size * sizeof(char*));
      free(names);
      names           = t;
      names[size - 1] = argv[i];
      names[size++]   = NULL;
    }
  }

  return runRequests(names, testIndex, membrk);
}