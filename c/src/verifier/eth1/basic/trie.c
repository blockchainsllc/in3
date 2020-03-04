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

#include "trie.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/sha3.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  uint8_t*     hash;
  trie_node_t* node;
} node_key_t;

static void _sha3(bytes_t* data, uint8_t* out) {
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, data->data, data->len);
  keccak_Final(&ctx, out);
}

static void finish_rlp(bytes_builder_t* bb, bytes_t* dst) {
  rlp_encode_to_list(bb);
  if (dst->data) _free(dst->data);
  dst->data = bb->b.data;
  dst->len  = bb->b.len;
  _free(bb);
}

static trie_codec_t rlp_codec = {.decode_item   = rlp_decode_in_list,
                                 .decode_size   = rlp_decode_len,
                                 .encode_add    = rlp_encode_item,
                                 .encode_finish = finish_rlp};

trie_t* trie_new() {
  trie_t* t              = _calloc(1, sizeof(trie_t));
  t->hasher              = _sha3;
  t->codec               = &rlp_codec;
  bytes_builder_t* ll    = bb_new();
  bytes_t          empty = bytes(NULL, 0);
  t->codec->encode_add(ll, &empty);
  t->hasher(&ll->b, t->root);
  bb_free(ll);
  return t;
}
void trie_free(trie_t* val) {
  trie_node_t *t = val->nodes, *p;
  while (t) {
    if (t->own_memory) _free(t->data.data);
    t = (p = t)->next;
    _free(p);
  }
  _free(val);
}

static void free_node(trie_node_t* n) {
  if (!n) return;
  if (n->own_memory) {
    // check if the node has a hash assigned. In this case it is stored and we be cleaned up later.
    int      l = 0;
    uint8_t* p = n->hash;
    for (; l < 32; l++, p++) {
      if (*p) return;
    }

    _free(n->data.data);
  }
  _free(n);
}
// -- trie_node --

static bytes_t trie_node_get_item(trie_node_t* t, int index) {
  bytes_t b = {.data = NULL, .len = 0};
  rlp_decode(&t->items, index, &b);
  return b;
}

static trie_node_t* trie_node_new(uint8_t* data, size_t len, uint8_t own_memory) {
  trie_node_t* t = _malloc(sizeof(trie_node_t));
  t->own_memory  = own_memory;
  t->data.data   = data;
  t->data.len    = len;
  memset(t->hash, 0, 32);
  rlp_decode(&t->data, 0, &t->items);

  switch (rlp_decode_len(&t->items)) {
    case 0: t->type = NODE_EMPTY; break;
    case 17: t->type = NODE_BRANCH; break;
    case 2: t->type = trie_node_get_item(t, 0).data[0] & 32 ? NODE_LEAF : NODE_EXT; break;
  }
  return t;
}

static void ensure_own_memory(trie_node_t* n) {
  if (n->own_memory) return;
  uint8_t* new_buffer = _malloc(n->data.len);
  memcpy(new_buffer, n->data.data, n->data.len);
  n->items.data = n->items.data - n->data.data + new_buffer;
  n->data.data  = new_buffer;
  n->own_memory = true;
}

static void trie_node_set_item(trie_node_t* t, int index, bytes_t* val, uint8_t is_list) {
  ensure_own_memory(t);
  bytes_builder_t* bb = bb_new();
  bytes_t          item;
  if (index == 0) {
    if (is_list)
      rlp_encode_list(bb, val);
    else
      rlp_encode_item(bb, val);
  } else {
    rlp_decode(&t->items, index - 1, &item);
    bb_write_raw_bytes(bb, t->items.data, item.data + item.len - t->items.data);
    if (is_list)
      rlp_encode_list(bb, val);
    else
      rlp_encode_item(bb, val);
  }

  rlp_decode(&t->items, index, &item);
  if (item.data + item.len < t->items.data + t->items.len)
    bb_write_raw_bytes(bb, item.data + item.len, t->items.data + t->items.len - item.data - item.len);

  t->items.len = bb->b.len;
  finish_rlp(bb, &t->data);
  t->items.data = t->data.data + t->data.len - t->items.len;
}

static trie_node_t* trie_node_create_branch(trie_t* trie, bytes_t* value) {
  bytes_builder_t* bb    = bb_new();
  bytes_t          empty = {.data = NULL, .len = 0};
  for (int i = 0; i < 16; i++) trie->codec->encode_add(bb, &empty);

  trie->codec->encode_add(bb, value ? value : &empty);
  trie->codec->encode_finish(bb, &empty);
  return trie_node_new(empty.data, empty.len, true);
}

static int nibble_len(uint8_t* val) {
  int i = 0;
  while (val[i] != 0xFF) i++;
  return i;
}

static int trie_node_value_from_nibbles(trie_node_type_t type, uint8_t* val, bytes_t* dst) {

  if (type == NODE_BRANCH || type == NODE_EMPTY) return -1;

  int      l = nibble_len(val), i, n, odd = l % 2;
  uint32_t blen = 1 + (l - odd) / 2;
  if (dst->len < blen || !dst->data) {
    if (dst->data) _free(dst->data);
    dst->data = _malloc(blen);
  }
  dst->len     = blen;
  dst->data[0] = ((type == NODE_EXT ? 0 : 2) + odd) << 4 | (odd == 0 ? 0 : val[0]);
  for (i = odd, n = 1; i < l; i += 2, n++) dst->data[n] = (val[i] << 4) | val[i + 1];
  return 0;
}

static void trie_node_set_path(trie_node_t* t, uint8_t* val) {
  bytes_t bytes = {.len = 0, .data = NULL};
  if (trie_node_value_from_nibbles(t->type, val, &bytes) == 0) {
    trie_node_set_item(t, 0, &bytes, false);
    _free(bytes.data);
  }
}

static trie_node_t* trie_node_create_leaf(trie_t* trie, uint8_t* nibbles, bytes_t* value) {
  bytes_builder_t* bb    = bb_new();
  bytes_t          empty = {.data = NULL, .len = 0};

  trie_node_value_from_nibbles(value->len ? NODE_LEAF : NODE_EXT, nibbles, &empty);
  trie->codec->encode_add(bb, &empty);
  trie->codec->encode_add(bb, value);
  trie->codec->encode_finish(bb, &empty);
  return trie_node_new(empty.data, empty.len, true);
}

static trie_node_t* trie_node_create_ext(trie_t* trie, uint8_t* nibbles, node_key_t target) {
  bytes_builder_t* bb    = bb_new();
  bytes_t          empty = {.data = NULL, .len = 0}, tmp = {.data = target.hash, .len = 32};
  trie_node_value_from_nibbles(NODE_EXT, nibbles, &empty);
  trie->codec->encode_add(bb, &empty);

  if (target.hash)
    rlp_encode_item(bb, &tmp);
  else {
    rlp_encode_list(bb, &target.node->items);
    free_node(target.node);
  }
  trie->codec->encode_finish(bb, &empty);
  return trie_node_new(empty.data, empty.len, true);
}

inline static node_key_t hash_key(uint8_t* hash) {
  node_key_t k = {.node = NULL, .hash = hash};
  return k;
}

inline static node_key_t node_key(trie_node_t* node) {
  node_key_t k = {.node = node, .hash = NULL};
  return k;
}

static node_key_t update_db(trie_t* t, trie_node_t* n, int top) {
  if (n->data.len < 32 && !top)
    return node_key(n);
  else {
    // do we already have it?
    trie_node_t* p = t->nodes;
    while (p) {
      if (p == n) break;
      p = p->next;
    }

    // we only add the node if we don't have it yet.
    if (p == NULL) {
      n->next  = t->nodes;
      t->nodes = n;
    }
    // update and return the hash
    _sha3(&n->data, n->hash);
    return hash_key(n->hash);
  }
}

static trie_node_t* get_node(trie_t* t, node_key_t key) {
  if (key.node) return key.node;
  trie_node_t* n = t->nodes;
  while (n != NULL) {
    if (memcmp(n->hash, key.hash, 32) == 0) return n;
    n = n->next;
  }
  return NULL;
}

static void set_node_target(trie_t* trie, trie_node_t* n, int index, node_key_t target) {
  if (target.node && target.node->data.len >= 32)
    target = update_db(trie, target.node, false);

  if (target.hash) {
    bytes_t tmp = {.data = target.hash, .len = 32};
    trie_node_set_item(n, index, &tmp, false);
  } else {
    trie_node_set_item(n, index, &target.node->items, true);
    free_node(target.node);
  }
}

static trie_node_t* get_node_target(trie_t* trie, trie_node_t* n, int index) {
  bytes_t tmp;
  // handle the next node
  if (rlp_decode(&n->items, index, &tmp) == 1) {
    // we have a hash and resolve the node
    return get_node(trie, hash_key(tmp.data));
  } else {
    bytes_t pre;
    rlp_decode(&n->items, index - 1, &pre);
    return trie_node_new(pre.data + pre.len, tmp.data + tmp.len - pre.data - pre.len, false);
  }
}

static node_key_t handle_node(trie_t* trie, trie_node_t* n, uint8_t* path, bytes_t* value, int top) {
  int          path_len  = nibble_len(path);
  uint8_t *    node_path = NULL, is_embedded, *rel_path = NULL;
  bytes_t      tmp;
  trie_node_t* b = NULL;

  if (!n) return update_db(trie, trie_node_create_leaf(trie, path, value), top);

  if (path_len == 0) {
    switch (n->type) {
      case NODE_EMPTY: // should not happen (only for the root, which is then simply null)
        break;
      case NODE_BRANCH:
        // here we simply change the value if the path ends here
        trie_node_set_item(n, 16, value, false);
        break;
      case NODE_LEAF:
        node_path = trie_path_to_nibbles(trie_node_get_item(n, 0), true);
        if (*node_path == 0XFF)
          // here we simply change the value if the path ends here
          trie_node_set_item(n, 2, value, false);
        else {
          // so this is a leaf with a longer path and we try to set a value with
          // here so we create a branch and set the leaf
          trie_node_set_path(n, node_path + 1);
          b = trie_node_create_branch(trie, value);
          set_node_target(trie, b, *node_path, update_db(trie, n, false));
          n = b;
        }
        break;
      case NODE_EXT:
        b           = trie_node_create_branch(trie, value);
        rel_path    = trie_path_to_nibbles(trie_node_get_item(n, 0), true);
        is_embedded = rlp_decode(&n->items, 1, &tmp) == 2;
        if (nibble_len(rel_path) == 1)
          // the extension has no elements to skip left, we remove it and replace it with the branch.
          trie_node_set_item(b, *rel_path, &tmp, is_embedded);
        else {
          // we remove the first nibble in the path with the branch
          trie_node_set_path(n, rel_path + 1);
          set_node_target(trie, b, *rel_path, update_db(trie, n, false));
        }
        n = b;
        break;
    }
  } else {
    uint8_t first = *path;
    int     matching;
    switch (n->type) {
      case NODE_EMPTY: break;                                                                          // should not happen (only for the root, which is then simply null)
      case NODE_BRANCH:                                                                                // for branches ...
        if (trie_node_get_item(n, first).len == 0)                                                     // if the next slot in the branch is empty...
          set_node_target(trie, n, first,                                                              // we can simply ...
                          update_db(trie, trie_node_create_leaf(trie, path + 1, value), false));       // create a leaf to the new value.
        else                                                                                           // .. so there is already something...
          set_node_target(trie, n, first,                                                              //  then we simply follow the path
                          handle_node(trie, get_node_target(trie, n, first), path + 1, value, false)); // and handle the next node
        break;

      case NODE_EXT:
      case NODE_LEAF:
        node_path = trie_path_to_nibbles(trie_node_get_item(n, 0), true);                                     // generate the path of the current node
        matching  = trie_matching_nibbles(node_path, path);                                                   // and check how many nibbles match the new one.
        if (matching == nibble_len(node_path)) {                                                              // next element fits exactly this this node
          if (n->type == NODE_EXT)                                                                            // in case of extension,
            set_node_target(trie, n, 1,                                                                       // we simply follohandle the target node
                            handle_node(trie, get_node_target(trie, n, 1), path + matching, value, false));   // after we handled its target
          else if (matching < path_len) {                                                                     // so this is a leaf, but it only matches partially
            tmp = trie_node_get_item(n, 1);                                                                   // we stor the current value in tmp
            b   = trie_node_create_branch(trie, &tmp);                                                        // and create a new branch with this value
            set_node_target(trie, b, path[matching],                                                          // and inside this branch ...
                            update_db(trie, trie_node_create_leaf(trie, path + matching + 1, value), false)); //  we create a leaf to the new value.
            if (*node_path != 0xFF) {                                                                         //  if this leaf ends here
              n->type = NODE_EXT;                                                                             // we have to change it to a extension
              trie_node_set_path(n, node_path);                                                               // with the same path
              set_node_target(trie, n, 1, update_db(trie, b, false));                                         // but now pointing to the branch as  target
            } else {                                                                                          // otherwise
              free_node(n);                                                                                   // we can remove it
              n = b;                                                                                          // and use the branch as the actual value
            }                                                                                                 // which will be updaded later
          } else                                                                                              // the path end here and it is a Leaf:
            trie_node_set_item(n, 1, value, false);                                                           //  so we can simply replace its value.

        } else {                                                                                          // does not fit, so we need rebuild the trie
          b        = trie_node_create_branch(trie, NULL);                                                 // we need a branch
          rel_path = path + matching;                                                                     // calculate the relative path to the current leaf
          if (*rel_path == 0xFF)                                                                          //  there is no path the leaf ends right in the branch,
            trie_node_set_item(b, 16, value, false);                                                      //  so we set the value
          else                                                                                            // we do have a relative path
            set_node_target(trie, b, *rel_path,                                                           // and set into the branch
                            node_key(trie_node_create_leaf(trie, rel_path + 1, value)));                  // the leaf node for our value.
          trie_node_set_path(n, node_path + matching + 1);                                                // the current node must change its path,
          set_node_target(trie, b, node_path[matching],                                                   // because it will become a child of the new branch
                          node_path[matching + 1] == 0xFF && n->type == NODE_EXT                          // if this is a extension and the path collapse now
                              ? node_key(get_node_target(trie, n, 1))                                     // we remove it and simply set the target directly in the branch
                              : update_db(trie, n, false));                                               // if not, we use the current extension or value
          if (matching) node_path[matching] = 0xFF;                                                       // do we need an extension to the branch?
          n        = matching > 0 ? trie_node_create_ext(trie, node_path, update_db(trie, b, false)) : b; // if so will simply insert one
          rel_path = NULL;                                                                                // we need to set it to null, since this rel_path does not point to new allocated memory and should not be freed.
        }
    }
  }

  if (node_path) _free(node_path); // clean up node_path, since this may have been created here
  if (rel_path) _free(rel_path);   // clean up rel_path, since this may have been created here
  return update_db(trie, n, top);  // update hash and store it (unless it is a embedded node)
}

void trie_set_value(trie_t* t, bytes_t* key, bytes_t* value) {
  if (key == NULL || value == NULL || value->len == 0 || key->len > 32) return;
  // create path based on the key
  uint8_t* path = trie_path_to_nibbles(*key, false);
  uint8_t* root = handle_node(t, get_node(t, hash_key(t->root)), path, value, true).hash;
  _free(path);
  memcpy(t->root, root, 32);
}

#ifdef TRIETEST
static void hexprint(uint8_t* a, int l) {
  (void) a; // unused param if compiled without debug
  int i;
  for (i = 0; i < l; i++) in3_log_trace("%02x", a[i]);
}
static void print_nibbles(bytes_t* path) {
  uint8_t *nibbles = trie_path_to_nibbles(*path, true), *p = nibbles;
  in3_log_trace(COLOR_GREEN);
  while (*p != 0xFF) {
    in3_log_trace("%01x", *p);
    p++;
  }
  in3_log_trace(COLOR_RESET);
  _free(nibbles);
}

static void dump_handle(trie_t* trie, trie_node_t* n, uint8_t with_hash, int level, char* prefix) {
  int     i;
  bytes_t tmp;
  char    _prefix[100];
  in3_log_trace("\n");
  for (i = 0; i < level; i++) in3_log_trace("  ");
  if (prefix) in3_log_trace("%s", prefix);
  if (n == NULL) {
    in3_log_trace("##MISSING NODE##");
    return;
  }
  if (with_hash) in3_log_trace("<%02x%02x%02x>", n->hash[0], n->hash[1], n->hash[2]);
  switch (n->type) {
    case NODE_BRANCH:
      in3_log_trace("" COLOR_YELLOW " ", "<BRANCH>");
      tmp = trie_node_get_item(n, 16);
      if (tmp.len) {
        in3_log_trace(" = ");
        hexprint(tmp.data, tmp.len);
      }
      for (i = 0; i < 16; i++) {
        if (rlp_decode(&n->items, i, &tmp) == 2) {
          sprintf(_prefix, "" COLOR_GREEN_X1 " : (EMBED) ", i);
          //          b_print(&tmp);
          trie_node_t* t = get_node_target(trie, n, i);
          dump_handle(trie, t, with_hash, level + 1, _prefix);
          _free(t);
        } else if (tmp.len) {
          sprintf(_prefix, "" COLOR_GREEN_X1 " : ", i);
          dump_handle(trie, get_node(trie, hash_key(tmp.data)), with_hash, level + 1, _prefix);
        }
      }
      break;
    case NODE_LEAF:
      in3_log_trace(COLOR_YELLOW_STR, "<LEAF");
      tmp = trie_node_get_item(n, 0);
      print_nibbles(&tmp);
      in3_log_trace(">");
      tmp = trie_node_get_item(n, 1);
      in3_log_trace(" = 0x");
      hexprint(tmp.data, tmp.len);
      break;
    case NODE_EXT:
      in3_log_trace(COLOR_YELLOW_STR, "<EXT ");
      tmp = trie_node_get_item(n, 0);
      print_nibbles(&tmp);
      in3_log_trace(">");

      if (rlp_decode(&n->items, 1, &tmp) == 2) {
        in3_log_trace(" ==> (EMBED) ");
        _prefix[0]     = 0;
        trie_node_t* t = get_node_target(trie, n, 1);
        dump_handle(trie, t, with_hash, level + 1, _prefix);
        _free(t);
      } else {
        in3_log_trace(" ==> ");
        _prefix[0] = 0;
        dump_handle(trie, get_node(trie, hash_key(tmp.data)), with_hash, level + 1, _prefix);
      }
      break;
    case NODE_EMPTY:
      in3_log_trace("<EMPTY>");
      break;
  }
}

void trie_dump(trie_t* trie, uint8_t with_hash) {
  in3_log_trace("\n\n root = ");
  hexprint(trie->root, 32);
  dump_handle(trie, get_node(trie, hash_key(trie->root)), with_hash, 2, "");
}
#endif