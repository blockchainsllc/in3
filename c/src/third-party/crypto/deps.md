#### trezor crypto

This library offers most crypto function used within the SDK.
files were mostly imported on an as-needed basis.

Changes:

  - base58.c:35 we need to include zephyr header file for it to enable alloca.h header
  - schnorr.c and schnorr.h are copied from a pending MR ( https://github.com/trezor/trezor-firmware/pull/93 )
  - rand.c removed the pragma warning


- current version: core/2.5.3
- src: [https://github.com/trezor/trezor-firmware/tree/master/crypto](https://github.com/trezor/trezor-firmware/tree/master/crypto)
- License: MIT ( check additional license headers in the src-files )

