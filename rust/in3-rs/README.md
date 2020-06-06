# in3-rs
Rust bindings for the Incubed C library.

The Incubed client is a
* Crypto-Economic
* Non-syncronizing and stateless, but fully verifying
* Minimal resource consuming

blockchain client (Crypto-Economic Client, Minimal Verification Client, Ultra Light Client).


**[API Documentation](https://docs.rs/in3-rs/)**


## Requirements
* Rust version >= 1.42.0
* in3-rs uses the in3-sys crate to provide low-level bindings to the IN3 C library. 
See the [in3-sys](https://github.com/slockit/in3-c/tree/master/rust/in3-sys) page for the requirements.


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
