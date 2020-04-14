use std::convert::TryInto;

use async_std::task;

use in3::{error, eth1};
use in3::prelude::*;

fn main() -> error::In3Result<()> {
    // configure client and API
    let mut eth_api = eth1::Api::new(Client::new(chain::MAINNET));
    eth_api.client.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);

    // call eth_blockNumber API method
    let latest_blk_num: u64 = task::block_on(eth_api.block_number()).unwrap().try_into().unwrap();
    println!("{:?}", latest_blk_num);
    Ok(())
}
