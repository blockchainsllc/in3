
OPTION(ZEPHYR "ZEPHYR" OFF)
IF(ZEPHYR)
  add_definitions(-DZEPHYR_OS)
  add_definitions(-DZEPHYR)
ENDIF(ZEPHYR)



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
	util/item.c
	util/mem.c
	util/stringbuilder.c
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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")