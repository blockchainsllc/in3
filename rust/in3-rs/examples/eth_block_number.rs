use std::convert::TryInto;

use async_std::task;

use in3::eth1::*;
use in3::prelude::*;

fn main() {
    let mut api = EthApi::new(Client::new(chain::MAINNET));
    api.client.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let num: u64 = task::block_on(api.block_number()).unwrap().try_into().unwrap();
    println!("{:?}", num);
}
