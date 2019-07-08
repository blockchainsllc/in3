
target_sources( app PRIVATE 
    ../core/client/context.c
    ../core/client/client.c
    ../core/client/cache.c
    ../core/client/nodelist.c
    ../core/client/verifier.c
    ../core/client/send.c
    ../core/client/client_init.c
    ../core/util/data.c
    ../core/util/debug.c
    ../core/util/bytes.c
    ../core/util/log.c
    ../core/util/scache.c
    ../core/util/utils.c
    ../core/util/mem.c
    ../core/util/stringbuilder.c
    ../core/crypto/ecdsa.c
    ../core/crypto/address.c
    ../core/crypto/bignum.c
    ../core/crypto/rand.c
    ../core/crypto/hmac.c
    ../core/crypto/secp256k1.c
    ../core/crypto/memzero.c
    ../core/crypto/sha3.c
    ../core/crypto/ripemd160.c
    ../core/crypto/base58.c
    ../core/crypto/hasher.c
    ../core/crypto/sha2.c
    ../core/crypto/blake256.c
    ../core/crypto/blake2b.c
    ../core/crypto/groestl.c
    
)
