#include "errors.h"

// definitions of hash functions
#define MH_H_SHA1 0x11
#define MH_H_SHA2_256 0x12
#define MH_H_SHA2_512 0x13
#define MH_H_SHA3_512 0x14
#define MH_H_SHA3_384 0x15
#define MH_H_SHA3_256 0x16
#define MH_H_SHA3_224 0x17
#define MH_H_SHAKE_128 0x18
#define MH_H_SHAKE_256 0x19
#define MH_H_BLAKE2B 0x40
#define MH_H_BLAKE2S 0x41


// list of avaliable hash functions.
static const int mh_all_hashes[] = {
	MH_H_SHA1,
	MH_H_SHA2_256,
	MH_H_SHA2_512,
	MH_H_SHA3_512,
	MH_H_SHA3_384,
	MH_H_SHA3_256,
	MH_H_SHA3_224,
	MH_H_SHAKE_128,
	MH_H_SHAKE_256,
	MH_H_BLAKE2B,
	MH_H_BLAKE2S,
};

#define MH_H_COUNT (int)(sizeof(mh_all_hashes) / sizeof(mh_all_hashes[0]))

const char *mh_hash_name(int hash);

// returns length in bytes or if returns is < 0 it is an error
int mh_hash_default_length(int hash);
