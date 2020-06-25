# in3-sys
Low-level unsafe Rust bindings for the Incubed C library.


**[API Documentation](https://docs.rs/in3-sys/)**

**NOTE**:
We recommend against using this crate directly.
Instead, consider using [in3-rs](https://github.com/slockit/in3-c/tree/master/rust/in3-rs), which provides a more high-level 'Rusty' interface.


## Requirements
* Rust version >= 1.42.0
* A toolchain capable of compiling IN3 C sources
  * Ubuntu: `apt-get install build-essential`
  * Windows: `https://visualstudio.microsoft.com/visual-cpp-build-tools/`
* OpenSSL dev libs
  * Ubuntu: `apt-get install libssl-dev`
  * Windows: *TODO*
* CMake version >= 3.5.1, because we build the bundled IN3 C library with the [`cmake` crate](https://github.com/alexcrichton/cmake-rs)
  * Ubuntu: `apt-get install cmake`
  * Windows: `https://cmake.org/install/`, *Make sure you add it to path for windows*
* [bindgen](https://github.com/rust-lang/rust-bindgen) and therefore clang dev libraries
  * Ubuntu: `apt-get install clang libclang-dev llvm-dev`
  * Windows: *TODO*
