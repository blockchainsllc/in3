#include <stdlib.h>

#include "hashes.h"

#include "errors.h"
#include "generic.h"


static const struct hash_info {
	int hash;
	const char *name;
	int length;
} hash_infos[] = {
	{ MH_H_SHA1, "sha1", 20},
	{ MH_H_SHA2_256, "sha2-256", 32 },
	{ MH_H_SHA2_512, "sha2-512", 64 },
	{ MH_H_SHA3_512, "sha3-512", 64 },
	{ MH_H_SHA3_384, "sha3-384", 45 },
	{ MH_H_SHA3_256, "sha3-256", 32 },
	{ MH_H_SHA3_224, "sha3-224", 28 },
	{ MH_H_SHAKE_128, "shake-128", 16 },
	{ MH_H_SHAKE_256, "shake-256", 32 },
	{ MH_H_BLAKE2B, "blake2b", 64 },
	{ MH_H_BLAKE2S, "blake2s", 32 }
};

mh_assert_static(sizeof(hash_infos) / sizeof(hash_infos[0]) == MH_H_COUNT);

// Searches for given hash in hash info table
static const struct hash_info *find_hash(int hash) {
	// naive search, could be replaced with binary
	unsigned int i = 0;
	for (; i < MH_H_COUNT; i++) {
		if (hash_infos[i].hash == hash)
			return &hash_infos[i];
	}

	return NULL;
}

const char *mh_hash_name(int hash) {
	const struct hash_info *info = find_hash(hash);
	return (info != NULL) ? info->name : NULL;
}


int mh_hash_default_length(int hash) {
	const struct hash_info *info = find_hash(hash);
	return (info != NULL) ? info->length : MH_E_UNKNOWN_CODE;
}
