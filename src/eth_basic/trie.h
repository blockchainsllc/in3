/** @file
 * Patricia Merkle Tree Imnpl
 * */
#include <util/bytes.h>
#ifndef in3_trie_h__
#define in3_trie_h__
/**
 *  hash-function
 */
typedef void (*in3_hasher_t)(bytes_t *src, bytes_t *dst);

/**
 *  codec to organize the encoding of the nodes
 */
typedef void (*in3_codec_add_t)(bytes_builder_t *bb, bytes_t *val);
typedef void (*in3_codec_finish_t)(bytes_builder_t *bb, bytes_t *dst);
typedef int (*in3_codec_decode_size_t)(bytes_t *src);
typedef int (*in3_codec_decode_index_t)(bytes_t *src, int index, bytes_t *dst);

/**
 * Node types.
 */
typedef enum {
  NODE_EMPTY  = 0, /**< empty node */
  NODE_BRANCH = 1, /**< a Branch */
  NODE_LEAF   = 2, /**< a leaf containing the value. */
  NODE_EXT    = 3  /**< a extension */
} trie_node_type_t;

/**
 * single node in the merkle trie.
 */
typedef struct trie_node {
  bytes_t           hash;        /**< the hash of the node */
  bytes_t           data;        /**< the raw data */
  bytes_t           items;       /**< the data as list  */
  uint8_t           is_embedded; /**< if true this is a embedded node */
  trie_node_type_t  type;        /**< type of the node */
  struct trie_node *next;        /**< used as linked list */
} trie_node_t;

/**
 *  the codec used to encode nodes.
 */
typedef struct trie_codec {
  in3_codec_add_t          encode_add;
  in3_codec_finish_t       encode_finish;
  in3_codec_decode_size_t  decode_size;
  in3_codec_decode_index_t decode_item;

} trie_codec_t;

/**
 * a merkle trie implementation.
 *
 * This is a Patricia Merkle Tree.
 */
typedef struct trie {
  in3_hasher_t  hasher; /**< hash-function. */
  trie_codec_t *codec;  /**< encoding of the nocds. */
  bytes_t       root;   /**< The root-hash. */
  trie_node_t * nodes;  /**< linked list of containes nodes */
} trie_t;

/**
 *  creates a new Merkle Trie.
 */
trie_t *trie_new();

/**
 * frees all resources of the trie.
 */
void trie_free(trie_t *val);

/**
 * sets a value in the trie.
 * The root-hash will be updated automaticly.
 */
void trie_set_value(trie_t *t, bytes_t *key, bytes_t *value);

#endif