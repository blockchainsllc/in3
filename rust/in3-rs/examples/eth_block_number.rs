// Make sure you build the in3 crate with blocking feature enabled to make this example work
// cargo run --example eth_block_number --features=blocking
extern crate in3;

use in3::prelude::*;

fn main() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err),
    }
}
