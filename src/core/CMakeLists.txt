
add_library( core STATIC 
	client/context.c
	client/client.c
	client/client_init.c
	util/debug.c
	util/bytes.c
	util/utils.c
	jsmn/jsmn.c
	jsmn/jsmnutil.c
	crypto/ecdsa.c
	crypto/address.c
	crypto/bignum.c
	crypto/rand.c
	crypto/hmac.c
	crypto/secp256k1.c
	crypto/memzero.c
	crypto/sha3.c
	crypto/ripemd160.c
	crypto/base58.c
	crypto/hasher.c
	crypto/sha2.c
	crypto/blake256.c
	crypto/blake2b.c
	crypto/groestl.c
)
set_property(TARGET core PROPERTY C_STANDARD 99)
