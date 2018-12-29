/** @file 
 * RLP-En/Decoding
 * */ 

#ifndef rlp_h__
#define rlp_h__

#include "../core/util/utils.h"

/*! this function returns the element with the given index by updating the reference of dst.
 * the bytes will only hold references and do not need to be freed! 
 * 
 * returns
 * - 0 : means item out of range
 * - 1 : item found
 * - 2 : list found ( you can then decode the same bytes again) 
 */
int rlp_decode(bytes_t* b, int index, bytes_t* dst);

/*!
* this function expects a list item (like the blockheader as first item and will then find the item within this list)
*/
int rlp_decode_in_list(bytes_t* b, int index, bytes_t* dst);

int  rlp_decode_len(bytes_t* b);
int  rlp_decode_item_len(bytes_t* b, int index);
int  rlp_decode_item_type(bytes_t* b, int index);


void rlp_encode_item(bytes_builder_t* bb, bytes_t* val);
void rlp_encode_list(bytes_builder_t* bb, bytes_t* val);
bytes_builder_t* rlp_encode_to_list(bytes_builder_t* bb);





#endif  // rlp__