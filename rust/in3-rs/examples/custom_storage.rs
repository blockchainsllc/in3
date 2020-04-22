extern crate in3;

use std::fs;

use in3::prelude::*;

struct FsStorage<'a> {
    dir: &'a str,
}

impl FsStorage<'_> {
    fn new(dir: &str) -> FsStorage {
        fs::create_dir_all(dir).unwrap();
        FsStorage { dir }
    }
}

impl Storage for FsStorage<'_> {
    fn get(&self, key: &str) -> Option<Vec<u8>> {
        println!("FsStorage log: get {}", key);
        match fs::read(format!("{}/{}", self.dir, key)) {
            Ok(value) => Some(value),
            Err(_) => None,
        }
    }

    fn set(&mut self, key: &str, value: &[u8]) {
        println!("FsStorage log: set {}", key);
        fs::write(format!("{}/{}", self.dir, key), value).expect("Unable to write file");
    }

    fn clear(&mut self) {
        println!("FsStorage log: clear");
        fs::remove_dir_all(format!("{}", self.dir)).unwrap();
    }
}

fn main() {
    let mut c = Client::new(chain::MAINNET);
    c.set_storage(Box::new(FsStorage::new("cache")));
    match c.rpc_blocking(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err),
    }
}
