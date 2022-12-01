#### libscrypt

scrypt is used as an encryption algorythm especially when decrypting private keys. (see `in3_decryptKey`). This library can be excluded by building with `-DSCRYPT=false`.

- version : [v1.21](https://github.com/technion/libscrypt/releases/tag/v1.21)
- License: BSD-2-Clause license
- changes:

we added a CMakeLists.txt in order to better inetgrate it and link it staticly.



