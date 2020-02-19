#include "multihash.h"

#include "hashes.h"
#include "errors.h"
#include "generic.h"

#include <string.h>
#include <stdlib.h>

#define VARINT_MASK (1 << 7)

static int check_len(size_t len) {
	if (len < 1)
		return MH_E_TOO_SHORT;
	else if (len >= 128)
		return MH_E_TOO_LONG;

	return MH_E_NO_ERROR;
}

static int check_multihash(const unsigned char mh[], size_t len) {
	int err;

	if (len < 3)
		return MH_E_TOO_SHORT;

	if (mh[0] & VARINT_MASK) {
		// This value is a varint, but there are currently no supported
		// values that require more than a single byte to represent.
		return MH_E_VARINT_NOT_SUPPORTED;
	} else if (mh[1] & VARINT_MASK) {
		return MH_E_VARINT_NOT_SUPPORTED;
	}

	err = check_len(mh[1]);
	if (err)
		return err;

	return 0;
}


// returns hash code or error (which is < 0)
int mh_multihash_hash(const unsigned char *mh, size_t len) {
	int err = check_multihash(mh, len);
	if (err)
		return err;

	return (int) mh[0];
}


// returns length of multihash or error (which is < 0)
int mh_multihash_length(const unsigned char *mh, size_t len) {
	int err = check_multihash(mh, len);
	if (err)
		return err;

	return (int) mh[1];
}

// gives access to raw digest inside multihash buffer
// returns 0 or negative error
int mh_multihash_digest(unsigned char *multihash, size_t len, unsigned char **digest,
		size_t *digest_len) {
	int err = check_multihash(multihash, len);
	if (err)
		return err;

	(*digest_len) = (size_t) mh_multihash_length(multihash, len);
	(*digest) = multihash + 2; // Always true without varint

	return 0;
}

int mh_new_length(int code, size_t hash_len) {
	// right now there is no varint support
	// so length required is 2 + hash_len
	UNUSED(code);
	return 2 + hash_len;
}

int mh_new(unsigned char *buffer, int code, const unsigned char *digest,
	size_t digest_len) {
	if (code & VARINT_MASK)
		return MH_E_VARINT_NOT_SUPPORTED;
	if (digest_len > 127)
		return MH_E_DIGSET_TOO_LONG;

	buffer[0] = (unsigned char) ((unsigned int) code) & 255;
	buffer[1] = (unsigned char) digest_len;
	memcpy(buffer + 2, digest, digest_len);

	return 0;
}

