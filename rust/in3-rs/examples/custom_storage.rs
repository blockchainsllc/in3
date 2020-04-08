extern crate in3;

use std::fs;

use in3::prelude::*;

const CACHE_PATH: &str = "cache";

fn main() {
    let mut c = Client::new(chain::MAINNET, false);
    fs::create_dir_all(CACHE_PATH).unwrap();
    c.set_storage(
        Box::new(|key| -> Vec<u8> {
            println!("get {}", key);
            match fs::read(format!("{}/{}", CACHE_PATH, key)) {
                Ok(value) => value,
                Err(_) => vec![],
            }
        }),
        Box::new(|key, value| {
            println!("set {} -> {:?}", key, value);
            fs::write(format!("{}/{}", CACHE_PATH, key), value).expect("Unable to write file");
        }),
        Box::new(|| {
            println!("clear");
            fs::remove_dir_all(format!("{}", CACHE_PATH)).unwrap();
        }),
    );
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err),
    }
}
