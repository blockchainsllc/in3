extern crate in3;

use async_std::task;

use in3::ipfs::*;
use in3::prelude::*;

fn main() {
    let mut ipfs_api = Api::new(Client::new(chain::IPFS));

    // `put` is an asynchronous request (due to the internal C library). Therefore to block
    // execution we use async_std's block_on function
    match task::block_on(ipfs_api.put("I love incubed".as_bytes().into())) {
        Ok(res) => println!("The hash is {:?}", res),
        Err(err) => println!("Failed with error: {:?}", err),
    }
}
