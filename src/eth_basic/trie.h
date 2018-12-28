/** @file 
 * Patricia Merkle Tree Imnpl
 * */ 
#include <util/bytes.h>
#ifndef in3_trie_h__
#define in3_trie_h__

typedef void (*in3_hasher_t)(bytes_t* src, bytes_t* dst);

typedef void (*in3_codec_add_t)(bytes_builder_t* bb, bytes_t* val);
typedef void (*in3_codec_finish_t)(bytes_builder_t* bb, bytes_t* dst);
typedef int (*in3_codec_decode_size_t)(bytes_t* src);
typedef int (*in3_codec_decode_index_t)(bytes_t* src, int index, bytes_t* dst);


typedef enum  {
	NODE_EMPTY = 0,
	NODE_BRANCH = 1,
	NODE_LEAF = 2,
 	NODE_EXT = 3
} trie_node_type_t;


typedef struct trie_node {
    bytes_t hash;
    bytes_t data;
    bytes_t items;
    uint8_t is_embedded;
    trie_node_type_t type;
    struct trie_node *next;
} trie_node_t;


typedef struct trie_codec {
    in3_codec_add_t     encode_add;
    in3_codec_finish_t  encode_finish;

    in3_codec_decode_size_t decode_size;
    in3_codec_decode_index_t decode_item;

} trie_codec_t;


typedef struct trie {
    in3_hasher_t hasher;
    trie_codec_t* codec;
    bytes_t root;
    trie_node_t* nodes;
} trie_t;

trie_t* trie_new();
void trie_free(trie_t* val);
void trie_set_value( trie_t* t, bytes_t* key, bytes_t* value );


#endif