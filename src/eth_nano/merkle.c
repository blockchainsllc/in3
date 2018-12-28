#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>  
#include <util/utils.h>
#include <util/mem.h>
//#include <zephyr.h>

//#include "fsm.h"
#include "merkle.h"
#include "rlp.h"

static int nibble_len(uint8_t *a) {
	int i=0;
	for (i=0;;i++) {
		if (a[i]==0xFF) return i;
	}
	return -1;
}


int matching_nibbles(uint8_t *a,uint8_t *b  ) {
	int i=0;
	for (i=0;;i++) {
		if (a[i]==0xff || b[i]==0xff || a[i]!=b[i]) return i;
	}
}

// converts the byte array to nibles of 4 bit each
uint8_t* str_to_nibbles(bytes_t *path, int use_prefix) {
	uint8_t *n = _malloc(1 + (path->len * 2));
	size_t j=0, i=0;
	for (; i < path->len; i++) {
		n[j++] = path->data[i] >> 4;
		n[j++] = path->data[i] & 0x0F;
		if (i==0 && use_prefix) 
		   n[0] = n[(j= n[0] & 1 ? 1 : 0)];
	}

	n[j] = 0xFF;
	return n;
}


static int check_node(bytes_t* raw_node, uint8_t** key, bytes_t* expectedValue, int is_last_node, bytes_t* last_value, uint8_t* next_hash ) {
  bytes_t node,val;
  rlp_decode(raw_node, 0, &node);
  int l = rlp_decode_len(&node);
  if (l==17) { // BRANCH
    if (**key==0xFF) {
        if (!is_last_node || rlp_decode(&node,16,&node)!=1) // if this is no the last node or the value is an embedded, which means more to come.
		  return 0;
		
		last_value->data = node.data;
		last_value->len = node.len;
		return 1;
	}

	if (rlp_decode(&node,**key,&val)==2) {
	   rlp_decode(&node,(**key)-1,&node);
       *key+=1;

		// we have an embedded node as next
	   node.data+=node.len;
	   node.len=val.data+val.len-node.data;
	   return check_node(&node,key,expectedValue,*(*key+1)==0xFF,last_value,next_hash);
	}
	else if (val.len!=32) // no hash, so we make sure the next hash is an empty hash
	    memset(next_hash, 0, 32);
	else
		memcpy(next_hash,val.data, 32);
	*key+=1;
	return 1;
  }
  else if (l==2) { // leaf or extension
	if (rlp_decode(&node,0,&val)!=1) return 0;
	uint8_t* path_nibbles = str_to_nibbles(&val, 1);
	int matching = matching_nibbles(path_nibbles, *key);
	int node_path_len = nibble_len(path_nibbles);
	int is_leaf = val.data[0] & 32;
   _free(path_nibbles);

    // if the relativeKey in the leaf does not math our rest key, we throw!
	if (node_path_len!=matching) {
        // so we have a wrong leaf here, if we actually expected this node to not exist,
        // the last node in this path may be a different leaf or a branch with a empty hash
        if (expectedValue == NULL && is_last_node)
          return 1;
		return 0;
	}

	*key+=node_path_len;
	if (rlp_decode(&node,1,&val)==2) { // this is an embedded node
	   rlp_decode(&node,0,&node);
	   node.data+=node.len;
	   node.len=val.data+val.len-node.data;
	   return check_node(&node,key,expectedValue,*(key+1)==NULL,last_value,next_hash);
	}
	else if (**key==0xFF) {
		if (!is_last_node) return 0;

		// if we are proven a value which shouldn't exist this must throw an error
		if (expectedValue == NULL && is_leaf)
		   return 0;
	}
	else if (is_leaf && expectedValue!=NULL)
	   return 0;
	
	last_value->data = val.data;
	last_value->len = val.len;
	memcpy(next_hash,val.data, 32); 

	return 1;
  }
  else { // empty node {
              // only if we expect no value we accept a empty node as last node
      if (expectedValue == NULL && is_last_node)
		  return 1;
	   return 0;
  }
  return 1;
}

/*
expectedValue == NULL     : value must not exist
expectedValue.data ==NULL : please copy the data I want to evaluate it afterwards.
expectedValue.data !=NULL : the value must match the data.
*/
int verifyMerkleProof(bytes_t* rootHash, bytes_t* path, bytes_t** proof, bytes_t* expectedValue) {
	int res=1;
	uint8_t* full_key = str_to_nibbles(path,0);
	uint8_t* key = full_key;
	uint8_t wanted_hash_data[32];
	bytes_t wanted_hash = { .len=32, .data=(uint8_t*)wanted_hash_data };
	memcpy(wanted_hash_data,rootHash->data,32);
	bytes_t* hash=NULL;
	bytes_t last_value;

	while (*proof) {
		// check hash
		hash = sha3(*proof);
		res=b_cmp(hash,&wanted_hash);
		b_free(hash);
		if (!res || !( res = check_node(*proof,&key, expectedValue, *(proof+1)==NULL,&last_value, wanted_hash_data))) break;
		proof+=1;
	}

	if (res && expectedValue!=NULL) {
		if (expectedValue->data==NULL) {
			if (last_value.data) {
				expectedValue->data = last_value.data;
				expectedValue->len  = last_value.len;
			}
		}
		else if (last_value.data==NULL || !b_cmp(expectedValue,&last_value)) {
			b_print(&last_value);
			b_print(expectedValue);
			res = 0;
		}
	}

	if (full_key)_free(full_key);
	return res;
}


//last: f9026a018302466db9010000000000000000000000200000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000400000000000000000000000000000000800000000000000000000000040000000000000000000000000000000000000010000000000000000000000000000080000000000000000000020000000000000000000000000000000080000000000010000000000000080000000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000f9015ff9015c9485ec283a3ed4b66df4da23656d4bf8a507383bcaf863a09123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8a0a2f7689fc12ea917d9029117d32b9fdef2a53462c853462ca86b71b97dd84af6a055a6ef49ec5dcf6cd006d21f151f390692eedd839c813a150000000000000000b8e00000000000000000000000009a75ea8f9934eb630c403dc14e11f27f5fbe7492000000000000000000000000000000000000000000000000000000005c1a511a000000000000000000000000000000000000000000000000000000005c1a598a000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000110d9316ec000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000815
//expe: f902048083000246b9010000000000000000000000200000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000400000000000000000000000000000000800000000000000000000000040000000000000000000000000000000000000010000000000000000000000000000080000000000000000000020000000000000000000000000000000080000000000010000000000000080000000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000f8faf8f89485ec283a3ed4b66df4da23656d4bf8a507383bcac0b8e00000000000000000000000009a75ea8f9934eb630c403dc14e11f27f5fbe7492000000000000000000000000000000000000000000000000000000005c1a511a000000000000000000000000000000000000000000000000000000005c1a598a000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000110d9316ec000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000815

void free_proof(bytes_t** proof) {
    bytes_t **p = proof;
    while (*p)
    {
        b_free(*p);
        p += 1;
    }
   _free(proof);
}