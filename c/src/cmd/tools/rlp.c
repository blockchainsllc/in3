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

/** @file 
 * simple commandline-util parsing json and creating bin
 * */

#include "../../verifier/eth1/nano/rlp.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/chainspec.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rlp_decode_item_type(bytes_t* b, int index) {
  bytes_t bb;
  return rlp_decode(b, index, &bb);
}

char* read_from_stdin(FILE* file) {
  if (file == NULL) {
    printf("File not found!");
    _Exit(1);
    return NULL;
  }

  size_t   allocated = 1024;
  size_t   len       = 0;
  uint8_t* buffer    = _malloc(1025);
  size_t   r;

  while (1) {
    r = fread(buffer + len, 1, allocated - len, file);
    len += r;
    if (feof(file)) break;
    size_t new_alloc = allocated * 2 + 1;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }

  if (len && buffer[len - 1] == '\n') buffer[len - 1] = 0;

  buffer[len] = 0;
  if (file != stdin) fclose(file);
  return (char*) buffer;
}
static char* ACCOUNT[] = {
    "ACCOUNT",
    "nonce",
    "balance",
    "storage hash",
    "code hash", 0};

static char* CHAINSPEC[] = {
    "CHAINSPEC",
    "version",
    "network id",
    "start nonce",
    "eip transitions",
    "consensus transitions", 0};

static char* BLOCK_HEADER[] = {
    "BLOCKHEADER",
    "parent hash",
    "sha3 uncles",
    "miner",
    "state root",
    "transaction root",
    "receipt root",
    "logs bloom",
    "difficulty",
    "number",
    "gas limit",
    "gas used",
    "timestamp",
    "extra data",
    "mixhash/proposerSeed",
    "nonce/signature",
    "extra sealed field", 0};

static char* TX[] = {
    "TRANSACTION",
    "nonce",
    "gas price",
    "gas",
    "to",
    "value",
    "data",
    "v",
    "r",
    "s", 0};

static char* TX_RECEIPT[] = {
    "TRANSACTION RECEIPT",
    "status",
    "cumulative gas",
    "logs bloom",
    "logs", 0};

static char* LOG[] = {
    "EVENT",
    "address",
    "topics",
    "data", 0};

static char* TRIE_LEAF[] = {
    "MERKLE LEAF",
    "path",
    "value", 0};

static char* EIP_TRANSITION[] = {
    "EIP TRANSITIONS",
    "block",
    "eips", 0};

static char* CONSENSUS_TRANSITION[] = {
    "CONSENSUS TRANSITIONS",
    "block",
    "type",
    "validators",
    "contract", 0};

static char* TRIE_BRANCH[] = {
    "MERKLE BRANCH",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "value", 0};

static int first = 1;

int alen(char** tt) {
  if (!tt) return 0;
  for (int i = 0; i < 20; i++) {
    if (tt[i] == NULL) return i - 1;
  }
  return 0;
}
void print_special(bytes_t data, char** ctx, int i) {
  if (ctx == EIP_TRANSITION && i % 2) {
    eip_t* eip = (eip_t*) data.data;
    printf("      EIP: ");
    if (eip->eip140) printf("140 ");
    if (eip->eip145) printf("145 ");
    if (eip->eip150) printf("150 ");
    if (eip->eip155) printf("155 ");
    if (eip->eip160) printf("160 ");
    if (eip->eip170) printf("170 ");
    if (eip->eip196) printf("196 ");
    if (eip->eip197) printf("197 ");
    if (eip->eip198) printf("198 ");
    if (eip->eip211) printf("211 ");
    if (eip->eip214) printf("214 ");
    if (eip->eip658) printf("658 ");
    if (eip->eip1014) printf("1014 ");
    if (eip->eip1052) printf("1052 ");
    if (eip->eip1283) printf("eip1283 ");
  }
  if (ctx == CONSENSUS_TRANSITION && i % 4 == 1) {
    if (data.len == 0)
      printf("     PoW Ethash");
    else if (*data.data == 1)
      printf("     PoA Aura");
    else if (*data.data == 2)
      printf("     PoA Clique");
  }
}

void write(bytes_t* data, char* l, char** tt) {
  bytes_t t;
  //  names
  int  al = alen(tt);
  char prefix[100];
  int  i, j, type, d;
  for (i = 0;; i++) {
    type = rlp_decode(data, i, &t);
    if (type == 0)
      return;
    else if (type < 0) {
      printf("Error: Decoding failed!\n");
      return;
    } else if (type == 1) {
      printf("%s", l);
      if (al && tt)
        printf("%-20s : ", tt[(i % al) + 1]);
      else
        printf("%-3i : ", i);

      if (tt == TRIE_LEAF && i == 0)
        d = printf("%s (%s)", (t.data[0] & 32) ? "LEAF" : "EXTENSION", (t.data[0] & 16) ? "odd" : "even");
      else if (t.len == 0)
        d = printf("0");
      else if (t.len < 9)
        d = printf("%" PRIu64 "", bytes_to_long(t.data, t.len));
      else if (t.len == 20)
        d = printf("<address>");
      else if (t.len == 32)
        d = printf("<hash>");
      else
        d = printf("<data %i>", t.len);

      for (j = d; j < 17; j++) printf(" ");
      if (t.len > 0)
        printf("0x");
      else
        printf("<EMPTY>");

      // split content
      int split_len    = 32;
      int split_prefix = 0;

      if (t.len % 20 == 0) split_len = 20;
      if (tt == TX && i == 5) {
        split_len    = 32;
        split_prefix = 4;
      }

      for (j = 0; j < (int) t.len; j++) {
        if (j > 0 && ((split_prefix && split_prefix == j) || (j - split_prefix) % split_len == 0) && (!tt || strcmp(tt[i + 1], "value"))) {
          printf("\n%s", l);
          if (tt) printf("%-20s  ", "");
          printf("%-17s 0x", "");
        }
        printf("%02x", t.data[j]);
      }

      // special values
      print_special(t, tt, i);

      printf("\n");

    } else if (type == 2) {
      int    l2 = rlp_decode_len(&t);
      char** t2 = NULL;
      if (tt == CHAINSPEC) {
        if (i == 3) t2 = EIP_TRANSITION;
        if (i == 4) t2 = CONSENSUS_TRANSITION;
      } else
        switch (l2) {
          case 15:
          case 16:
            t2 = BLOCK_HEADER;
            break;
          case 17:
            t2 = TRIE_BRANCH;
            break;
          case 2:
            t2 = TRIE_LEAF;
            break;
          case 9:
            t2 = TX;
            break;
          case 5:
            t2 = CHAINSPEC;
            break;
          case 3:
            t2 = LOG;
            break;
          case 4:
            t2 = rlp_decode_item_type(&t, 3) == 2 ? TX_RECEIPT : ACCOUNT;
            break;
        }
      if (tt && tt != CHAINSPEC) t2 = NULL;
      printf("%s", l);
      if (tt)
        printf("%-20s : ", tt[i + 1]);

      printf("[ %s", t2 ? t2[0] : "");

      if (first) {
        bytes_t* hash = sha3(data);
        printf("  Hash : 0x");
        first = 0;

        for (int j = 0; j < 32; j++)
          printf("%02x", hash->data[j]);

        b_free(hash);
      }
      printf("\n");

      sprintf(prefix, "%s   ", l);
      write(&t, prefix, t2);
      printf("%s]\n", l);
    }
  }
}

void add_rlp(bytes_builder_t* bb, char* val) {
  int l = strlen(val);
  if (l > 1 && val[0] == '0' && val[1] == 'x') {
    bytes_t* b = hex_to_new_bytes(val + 2, l - 2);
    rlp_encode_item(bb, b);
    b_free(b);
  } else {
    uint8_t  data[8];
    uint64_t value = _strtoull(val, NULL, 10);
    bytes_t  bytes = {.len = 0, .data = data};
    if (value) {
      bytes.len  = min_bytes_len(value);
      bytes.data = data + 8 - min_bytes_len(value);
      long_to_bytes(value, data);
    }
    rlp_encode_item(bb, &bytes);
  }
}

int main(int argc, char* argv[]) {
  char*            input  = NULL;
  bytes_builder_t* bb     = bb_new();
  int              output = 0;

  int i;

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0)
      input = read_from_stdin(fopen(argv[++i], "r"));
    else if (strcmp(argv[i], "-e") == 0)
      output = 1;
    else if (strcmp(argv[i], "-l") == 0)
      output = 2;
    else if (output)
      add_rlp(bb, argv[i]);
    else if (input) {
      input = _realloc(input, strlen(input) + 1 + strlen(argv[i]), strlen(input) + 1);
      strcat(input, argv[i]);
    } else
      input = argv[i];
  }

  if (output) {
    if (output == 2)
      rlp_encode_to_list(bb);
    printf("0x");
    for (int j = 0; j < (int) bb->b.len; j++)
      printf("%02x", bb->b.data[j]);

    printf("\n");

    return 0;
  }

  if (input == NULL) input = read_from_stdin(stdin);

  bytes_t* bytes;
  if (input[0] == '0' && input[1] == 'x') input += 2;
  if (*input == '\"') {
    int la        = strlen(input);
    input[la - 1] = 0;
    la -= 2;
    input++;
    for (int i = 0; input[i]; i++) {
      if (input[i] == '\"') {
        memmove(input + i, input + i + 3, la - i - 2);
        la -= 3;
      }
    }
  }
  if (strchr(input, '\\')) {
    bytes       = _malloc(sizeof(bytes_t));
    bytes->data = _malloc(strlen(input));
    bytes->len  = 0;

    for (int n = 0; input[n]; n++) {
      if (input[n] == '\\' && input[n + 1] == 'x') {
        bytes->data[bytes->len++] = hexchar_to_int(input[n + 2]) << 4 | hexchar_to_int(input[n + 3]);
        n += 3;
      } else
        bytes->data[bytes->len++] = input[n];
    }
  } else if (*input == ':') {
    bytes                     = hex_to_new_bytes(input + 1, strlen(input + 1));
    chain_id_t       chain_id = bytes_to_long(bytes->data, bytes->len);
    chainspec_t*     spec     = chainspec_get(chain_id);
    bytes_builder_t* bb       = bb_new();
    chainspec_to_bin(spec, bb);
    bytes = &bb->b;
  } else
    bytes = hex_to_new_bytes(input, strlen(input));

  write(bytes, "", NULL);

  return 0;
}