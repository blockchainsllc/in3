#ifndef MH_HULTIHASH_H
#define MH_HULTIHASH_H

#include <stddef.h>

// returns hash code or error (which is < 0)
int mh_multihash_hash(const unsigned char *multihash, size_t len);

// returns length of multihash or error (which is < 0)
int mh_multihash_length(const unsigned char *multihash, size_t len);

// gives access to raw digest inside multihash buffer
// returns 0 or negative error
int mh_multihash_digest(unsigned char *multihash, size_t len,
	 unsigned char **digest, size_t *digest_len);

// returns length in bytes of buffer needed to store multihash
// with given hashcode and with given digest length
// returns length or negative error code
int mh_new_length(int code, size_t digest_len);

// writes multihash into a buffer, the buffer needs to be at least
// mh_new_length() bytes long.
// returns negative error code or 0
int mh_new(unsigned char *buffer, int code, const unsigned char *digest,
	size_t digest_len);

#endif /* end of include guard */
