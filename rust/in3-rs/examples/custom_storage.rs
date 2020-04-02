extern crate in3;

use in3::prelude::*;

fn main() {
    let mut c = Client::new(chain::MAINNET);
    c.set_storage(Box::new(|key| -> Vec<u8>{
        println!("get {}", key);
        return vec![];
    }), Box::new(|key, val| {
        println!("set {} -> {:?}", key, val);
    }), Box::new(|| {
        println!("clear");
    }));
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}