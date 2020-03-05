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
 * Patricia Merkle Tree Imnpl
 * */
#include "../../../core/util/bytes.h"
#ifndef in3_trie_h__
#define in3_trie_h__
/**
 *  hash-function
 */
typedef void (*in3_hasher_t)(bytes_t* src, uint8_t* dst);

/**
 *  codec to organize the encoding of the nodes
 */
typedef void (*in3_codec_add_t)(bytes_builder_t* bb, bytes_t* val);
typedef void (*in3_codec_finish_t)(bytes_builder_t* bb, bytes_t* dst);
typedef int (*in3_codec_decode_size_t)(bytes_t* src);
typedef int (*in3_codec_decode_index_t)(bytes_t* src, int index, bytes_t* dst);

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
  uint8_t           hash[32];   /**< the hash of the node */
  bytes_t           data;       /**< the raw data */
  bytes_t           items;      /**< the data as list  */
  uint8_t           own_memory; /**< if true this is a embedded node with own memory */
  trie_node_type_t  type;       /**< type of the node */
  struct trie_node* next;       /**< used as linked list */
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
  trie_codec_t* codec;  /**< encoding of the nocds. */
  bytes32_t     root;   /**< The root-hash. */
  trie_node_t*  nodes;  /**< linked list of containes nodes */
} trie_t;

/**
 *  creates a new Merkle Trie.
 */
trie_t* trie_new();

/**
 * frees all resources of the trie.
 */
void trie_free(trie_t* val);

/**
 * sets a value in the trie.
 * The root-hash will be updated automaticly.
 */
void trie_set_value(trie_t* t, bytes_t* key, bytes_t* value);

#ifdef TEST
void trie_dump(trie_t* trie, uint8_t with_hash);
#endif
#endif