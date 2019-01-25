#include "trie.h"
#include <crypto/sha3.h>
#include <merkle.h>
#include <rlp.h>
#include <stdio.h>
#include <string.h>
#include <util/mem.h>
#include <util/utils.h>
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
  bytes_t          empty = b_as_bytes(NULL, 0);
  t->codec->encode_add(ll, &empty);
  t->hasher(&ll->b, t->root);
  bb_free(ll);
  return t;
}
void trie_free(trie_t* val) {
  trie_node_t *t = val->nodes, *p;
  while (t) {
    _free(t->data.data);
    t = (p = t)->next;
    _free(p);
  }
  _free(val);
}

// -- trie_node --

static bytes_t trie_node_get_item(trie_node_t* t, int index) {
  bytes_t b;
  rlp_decode(&t->items, index, &b);
  return b;
}

static trie_node_t* trie_node_new(uint8_t* data, size_t len,
                                  uint8_t is_embedded) {
  trie_node_t* t = _malloc(sizeof(trie_node_t));
  t->data.data   = data;
  t->data.len    = len;
  rlp_decode(&t->data, 0, &t->items);

  switch (rlp_decode_len(&t->items)) {
    case 0: t->type = NODE_EMPTY; break;
    case 17: t->type = NODE_BRANCH; break;
    case 2:
      t->type = trie_node_get_item(t, 0).data[0] & 32 ? NODE_LEAF : NODE_EXT;
      break;
  }
  t->is_embedded = is_embedded;
  return t;
}

static void trie_node_set_item(trie_node_t* t, int index, bytes_t* val) {
  bytes_builder_t* bb = bb_new();
  bytes_t          item;
  if (index == 0)
    rlp_encode_item(bb, val);
  else {
    rlp_decode(&t->items, index - 1, &item);
    bb_write_raw_bytes(bb, t->items.data, item.data + item.len - t->items.data);
    rlp_encode_item(bb, val);
  }

  rlp_decode(&t->items, index, &item);
  if (item.data + item.len < t->items.data + t->items.len)
    bb_write_raw_bytes(bb, item.data + item.len,
                       t->items.data + t->items.len - item.data - item.len);

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
  return trie_node_new(empty.data, empty.len, false);
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
  if (dst->len < blen) {
    if (dst->data) _free(dst->data);
    dst->data = _malloc(blen);
  }
  dst->len     = blen;
  dst->data[0] = (type == NODE_EXT ? 0 : 2 + odd) << 4 | (odd == 0 ? 0 : val[0]);
  for (i = odd, n = 1; i < l; i += 2, n++) dst->data[n] = (val[i] << 4) | val[i + 1];
  return 0;
}

static void trie_node_set_path(trie_node_t* t, uint8_t* val) {
  bytes_t bytes = {.len = 0, .data = NULL};
  if (trie_node_value_from_nibbles(t->type, val, &bytes) == 0) {
    trie_node_set_item(t, 0, &bytes);
    _free(bytes.data);
  }
}

static trie_node_t* trie_node_create_leaf(trie_t* trie, uint8_t* nibbles, bytes_t* value) {
  bytes_builder_t* bb    = bb_new();
  bytes_t          empty = {.data = NULL, .len = 0};

  trie_node_value_from_nibbles(NODE_LEAF, nibbles, &empty);
  trie->codec->encode_add(bb, &empty);
  trie->codec->encode_add(bb, value);
  trie->codec->encode_finish(bb, &empty);
  return trie_node_new(empty.data, empty.len, false);
}

static trie_node_t* trie_node_create_ext(trie_t* trie, uint8_t* nibbles,
                                         bytes_t* value) {
  bytes_builder_t* bb    = bb_new();
  bytes_t          empty = {.data = NULL, .len = 0};
  trie_node_value_from_nibbles(NODE_EXT, nibbles, &empty);
  trie->codec->encode_add(bb, &empty);
  trie->codec->encode_add(bb, value);
  trie->codec->encode_finish(bb, &empty);
  return trie_node_new(empty.data, empty.len, false);
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

static bytes_t key_as_data(node_key_t k) {
  return k.hash ? b_as_bytes(k.hash, 32) : k.node->data;
}

static node_key_t handle_node(trie_t* trie, trie_node_t* n, uint8_t* path,
                              bytes_t* value, int top) {
  if (!n) return update_db(trie, trie_node_create_leaf(trie, path, value), top);
  int          path_len  = nibble_len(path);
  uint8_t*     node_path = NULL;
  uint8_t*     rel_path  = NULL;
  trie_node_t *b = NULL, *b2 = NULL;

  bytes_t tmp;
  if (path_len == 0) {
    switch (n->type) {
      case NODE_EMPTY: // should not happen (only for the root, which is then
                       // simply null)
        break;
      case NODE_BRANCH:
        // here we simply change the value if the path ends here
        trie_node_set_item(n, 16, value);
        break;
      case NODE_LEAF:
        tmp       = trie_node_get_item(n, 0);
        node_path = trie_path_to_nibbles(&tmp, true);
        if (*node_path == 0XFF)
          // here we simply change the value if the path ends here
          trie_node_set_item(n, 2, value);
        else {
          // so this is a leaf with a longer path and we try to set a value with
          // here so we create a branch and set the leaf
          trie_node_set_path(n, path + 1);
          b   = trie_node_create_branch(trie, value);
          tmp = key_as_data(update_db(trie, n, false));
          trie_node_set_item(n, *node_path, &tmp);
          n = b;
        }
        break;
      case NODE_EXT:
        b        = trie_node_create_branch(trie, value);
        tmp      = trie_node_get_item(n, 0);
        rel_path = trie_path_to_nibbles(&tmp, true);
        tmp      = trie_node_get_item(n, 1);
        if (nibble_len(rel_path) == 1)
          // the extension has no elements to skip left, we remove it and
          // replace it with the branch.
          trie_node_set_item(b, *rel_path, &tmp);
        else {
          // we remove the first nibble in the path with the branch
          trie_node_set_path(n, rel_path + 1);
          tmp = key_as_data(update_db(trie, n, false));
          // update the hash here, since we return the hash of the branch
          trie_node_set_item(b, *rel_path, &tmp);
        }
        n = b;
        break;
    }
  } else {
    uint8_t first = *path;
    int     matching;
    switch (n->type) {
      case NODE_EMPTY: // should not happen (only for the root, which is then
                       // simply null)
        break;
      case NODE_BRANCH:
        if (trie_node_get_item(n, first).len == 0) {
          tmp = key_as_data(update_db(trie, trie_node_create_leaf(trie, path + 1, value), false));
          // we can simply add a leaf here
          trie_node_set_item(n, first, &tmp);
        } else {
          // handle the next node
          b = rlp_decode(&n->items, first, &tmp) == 1
                  ? get_node(trie, hash_key(tmp.data))
                  : trie_node_new(tmp.data, tmp.len, true);

          tmp = key_as_data(handle_node(trie, b, path + 1, value, false));
          trie_node_set_item(n, first, &tmp);
        }
        break;

      case NODE_EXT:
      case NODE_LEAF:
        tmp       = trie_node_get_item(n, 0);
        node_path = trie_path_to_nibbles(&tmp, true);
        matching  = trie_matching_nibbles(node_path, path);
        if (matching ==
            nibble_len(
                path)) {            // next element fits so we can update next node
          if (n->type == NODE_LEAF) // for Leaf: we simply replace the leaf-value
            trie_node_set_item(n, 1, value);
          else { // for Extension: we follow the path
            b = rlp_decode(&n->items, 1, &tmp) == 1
                    ? get_node(trie, hash_key(tmp.data))
                    : trie_node_new(tmp.data, tmp.len, true);

            tmp = key_as_data(handle_node(trie, b, path + matching, value, false));
            trie_node_set_item(n, 1, &tmp);
          }
        } else { // does not fit, so we need rebuild the trie
          b = trie_node_create_branch(trie, NULL);
          trie_node_set_path(n, node_path + matching + 1);
          rel_path = path + matching;
          if (*rel_path == 0xFF)
            trie_node_set_item(b, 16, value);
          else {
            tmp = key_as_data(update_db(
                trie, trie_node_create_leaf(trie, rel_path + 1, value),
                false));
            trie_node_set_item(b, *rel_path, &tmp);
          }

          tmp = key_as_data(node_path[matching + 1] == 0xFF && n->type == NODE_EXT
                                ? node_key(rlp_decode(&n->items, 1, &tmp) == 1
                                               ? get_node(trie, hash_key(tmp.data))
                                               : (b2 = trie_node_new(
                                                      tmp.data, tmp.len, true)))
                                : update_db(trie, n, false));

          trie_node_set_item(b, node_path[matching], &tmp);

          if (matching) node_path[matching] = 0xFF;
          // use the new current node
          if (matching > 0) {
            tmp = key_as_data(update_db(trie, b, false));
            n   = trie_node_create_ext(trie, node_path, &tmp);
          } else
            n = b;
          rel_path = NULL;
        }
    }
  }

  // update hash and store it
  node_key_t key = update_db(trie, n, top);

  // clean up
  if (b && b->is_embedded)
    _free(b);
  if (b2 && b2->is_embedded)
    _free(b2);
  if (node_path) _free(node_path);
  if (rel_path) _free(rel_path);

  return key;
}

void trie_set_value(trie_t* t, bytes_t* key, bytes_t* value) {
  //    printf("set key/value\n");
  //    b_print(key);
  //    b_print(value);
  uint8_t* path = trie_path_to_nibbles(key, false);
  uint8_t* root =
      handle_node(t, get_node(t, hash_key(t->root)), path, value, true).hash;
  _free(path);
  memcpy(t->root, root, 32);
  //    printf("   root=");
  //    b_print(root);
}

#ifdef TEST
static void hexprint(uint8_t* a, int l) {
  int i;
  for (i = 0; i < l; i++) printf("%02x", a[i]);
}

static void dump_handle(trie_t* trie, trie_node_t* n, uint8_t with_hash, int level, char* prefix) {
  int     i;
  bytes_t tmp;
  char    _prefix[100];
  printf("\n");
  for (i = 0; i < level; i++) printf("  ");
  if (prefix) printf("%s", prefix);
  if (with_hash) printf("<%02x%02x%02x>", n->hash[0], n->hash[1], n->hash[2]);
  switch (n->type) {
    case NODE_BRANCH:
      printf("<BRANCH> ");
      tmp = trie_node_get_item(n, 16);
      if (tmp.len) {
        printf(" = ");
        hexprint(tmp.data, tmp.len);
      }
      for (i = 0; i < 16; i++) {

        if (rlp_decode(&n->items, i, &tmp) == 2) {
          sprintf(_prefix, " %02x : (EMBED) ", i);
          trie_node_t* t = trie_node_new(tmp.data, tmp.len, true);
          dump_handle(trie, t, with_hash, level + 1, _prefix);
          _free(t);
        } else if (tmp.len) {
          sprintf(_prefix, " %02x : ", i);
          dump_handle(trie, get_node(trie, hash_key(tmp.data)), with_hash, level + 1, _prefix);
        }
      }
      break;
    case NODE_LEAF:
      printf("<LEAF> ");
      tmp = trie_node_get_item(n, 0);
      hexprint(tmp.data, tmp.len);
      tmp = trie_node_get_item(n, 1);
      printf(" = 0x");
      hexprint(tmp.data, tmp.len);
      break;
    case NODE_EXT:
      printf("<EXT> ");
      tmp = trie_node_get_item(n, 0);
      hexprint(tmp.data, tmp.len);
      if (rlp_decode(&n->items, i, &tmp) == 2) {
        sprintf(_prefix, " ==> (EMBED) ");
        trie_node_t* t = trie_node_new(tmp.data, tmp.len, true);
        dump_handle(trie, t, with_hash, level + 1, _prefix);
        _free(t);
      } else {
        sprintf(_prefix, " ==> ");
        dump_handle(trie, get_node(trie, hash_key(tmp.data)), with_hash, level + 1, _prefix);
      }
      break;
    case NODE_EMPTY:
      printf("<EMPTY>");
      break;
  }
}
void trie_dump(trie_t* trie, uint8_t with_hash) {
  printf("\n\n root = ");
  hexprint(trie->root, 32);
  dump_handle(trie, get_node(trie, hash_key(trie->root)), with_hash, 0, "");
}
#endif