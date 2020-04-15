use std::convert::TryInto;

use async_std::task;
use ethereum_types::Address;

use in3::eth1;
use in3::prelude::*;

fn main() -> In3Result<()> {
    // configure client and API
    let mut eth_api = eth1::Api::new(Client::new(chain::MAINNET));
    eth_api.client().configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;

    // eth_blockNumber
    let latest_blk_num: u64 = task::block_on(eth_api.block_number())?.try_into().unwrap();
    println!("Latest block number is {:?}", latest_blk_num);

    // eth_getBalance
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let balance: u64 = task::block_on(eth_api.get_balance(address, eth1::BlockNumber::Number((latest_blk_num - 10).into())))?.try_into().unwrap();
    println!("Balance of address {:?} is {:?} wei", address, balance);

    // eth_getBlockByNumber
    let block: eth1::Block = task::block_on(eth_api.get_block_by_number(eth1::BlockNumber::Latest, false))?;
    println!("Block => {:?}", block);
    Ok(())
}
