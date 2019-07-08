# main incubed module defining the interfaces for transport, verifier and storage. 
#
# This module does not have any dependencies and cannot be used without additional modules providing verification and transport.


add_library( core STATIC 
	client/context.c
	client/client.c
	client/cache.c
	client/nodelist.c
	client/verifier.c
	client/send.c
	client/client_init.c
	util/debug.c
	util/bytes.c
	util/utils.c
	util/scache.c
	util/data.c
	util/log.c
	util/mem.c
	util/stringbuilder.c
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
	crypto/pbkdf2.c
)
include(compiler.cmake)
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")