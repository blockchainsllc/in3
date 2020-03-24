extern crate in3;

use in3::prelude::*;

fn main() {
    let mut c = Client::new(ChainId::Mainnet);
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}