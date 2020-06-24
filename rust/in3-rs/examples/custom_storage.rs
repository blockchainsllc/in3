extern crate in3;

use std::fs;

use async_std::task;

use in3::prelude::*;

struct FsStorage<'a> {
    dir: &'a str,
}

impl FsStorage<'_> {
    fn new(dir: &str) -> FsStorage {
        fs::create_dir_all(dir).expect(format!("failed to create {}", dir).as_str());
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
        fs::write(format!("{}/{}", self.dir, key), value).expect("unable to write file");
    }

    fn clear(&mut self) {
        println!("FsStorage log: clear");
        fs::remove_dir_all(format!("{}", self.dir))
            .expect(format!("failed to remove {}", self.dir).as_str());
    }
}

fn main() {
    let mut c = Client::new(chain::MAINNET);
    c.set_storage(Box::new(FsStorage::new("cache")));
    match task::block_on(c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#)) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("Failed with error: {}", err),
    }
}
