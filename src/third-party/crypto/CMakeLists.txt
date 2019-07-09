add_library( crypto STATIC 
	ecdsa.c
	address.c
	bignum.c
	rand.c
	hmac.c
	secp256k1.c
	memzero.c
	sha3.c
	ripemd160.c
	base58.c
	hasher.c
	sha2.c
	blake256.c
	blake2b.c
	groestl.c
	pbkdf2.c
)
