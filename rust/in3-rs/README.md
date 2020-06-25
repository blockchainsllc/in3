# in3-rs
Rust bindings for the Incubed C library.

The Incubed client is a
* Crypto-Economic
* Non-syncronizing and stateless, but fully verifying
* Minimal resource consuming

blockchain client (Crypto-Economic Client, Minimal Verification Client, Ultra Light Client).

```toml
[dependencies]
in3 = "0.0.2"
```

Links:
* **[Crate](https://crates.io/crates/in3)**
* **[API](https://docs.rs/in3/)**

## Requirements
* Rust compiler version >= 1.42.0 -> `rustc --version`
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

> in3-rs uses the [`in3-sys`](https://github.com/slockit/in3-c/tree/master/rust/in3-sys) to provide low-level bindings to the IN3 C library. 


## Features
* `blocking`- Enables the blocking API which depends on [async-std](https://github.com/async-rs/async-std).

## Example
```rust
use std::convert::TryInto;
use async_std::task;

use in3::eth1::*;
use in3::prelude::*;

fn main() -> In3Result<()> {
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    eth_api.client().configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;

    let latest_blk_num: u64 = task::block_on(eth_api.block_number())?.try_into()?;
    println!("Latest block number is {:?}", latest_blk_num);
    Ok(())
}
```

## Contributing

#### Instructions for running the tests

1. To run the tests you need to first run the build script. `./scripts/build_rust.sh`.
2. Navigate to the rust binding folder. `cd rust`
3. Then run the test using **cargo** and an additional option `RUST_TEST_THREADS=1`. This can be done via `RUST_TEST_THREADS=1 cargo test`.
