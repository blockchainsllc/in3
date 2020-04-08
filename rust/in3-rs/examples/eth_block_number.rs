extern crate in3;

use in3::prelude::*;

fn main() {
    let mut c = Client::new(chain::MAINNET, false);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err),
    }
}
