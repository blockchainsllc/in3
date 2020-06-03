# in3-sys
Low-level unsafe Rust bindings for the Incubed C library.


**[API Documentation](https://docs.rs/in3-sys/)**

**NOTE**:
We recommend against using this crate directly.
Instead, consider using [in3-rs](https://github.com/slockit/in3-c/tree/master/rust/in3-rs), which provides a more high-level 'Rusty' interface.


## Requirements
* Rust version >= 1.42.0
* A toolchain capable of compiling IN3 C sources
* OpenSSL dev libs (`apt-get install libssl-dev` on Ubuntu)
* CMake version >= 3.5.1, because we build the bundled IN3 C library with the [`cmake` crate](https://github.com/alexcrichton/cmake-rs)
* [bindgen](https://github.com/rust-lang/rust-bindgen) and therefore clang dev libraries (`apt-get install clang libclang-dev llvm-dev` on Ubuntu)
