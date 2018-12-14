#ifndef rlp_h__
#define rlp_h__

#include "../core/util/utils.h"

/*! this function returns the element with the given index by updating the reference of dst.
 * the bytes will only hold references and do not need to be freed! 
 * 
 * return 
 * 0 : means item out of range
 * 1 : item found
 * 2 : list found ( you can then decode the same bytes again) 
 */
int rlp_decode_item(bytes_t* b, size_t index, bytes_t* dst);

#endif  // rlp__