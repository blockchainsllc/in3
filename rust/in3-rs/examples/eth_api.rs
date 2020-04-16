use std::convert::TryInto;

use async_std::task;
use ethereum_types::{Address, U256};

use in3::eth1;
use in3::prelude::*;
use in3::types::Bytes;

fn main() -> In3Result<()> {
    // configure client and API
    let mut eth_api = eth1::Api::new(Client::new(chain::MAINNET));
    eth_api.client().configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;

    // eth_getStorageAt
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let key: U256 = 0u64.into();
    let storage: u64 = task::block_on(eth_api.get_storage_at(address, key, eth1::BlockNumber::Latest))?.try_into().unwrap();
    println!("Storage value is {:?}", storage);

    // eth_getCode
    let address: Address = serde_json::from_str(r#""0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f""#)?;
    let code: Bytes = task::block_on(eth_api.get_code(address, eth1::BlockNumber::Latest))?.try_into().unwrap();
    println!("Code at address {:?} is {:?}", address, code);

    // eth_blockNumber
    let latest_blk_num: u64 = task::block_on(eth_api.block_number())?.try_into().unwrap();
    println!("Latest block number is {:?}", latest_blk_num);

    // eth_gasPrice
    let gas_price: u64 = task::block_on(eth_api.gas_price())?.try_into().unwrap();
    println!("Gas price is {:?}", gas_price);

    // eth_getBalance
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let balance: u64 = task::block_on(eth_api.get_balance(address, eth1::BlockNumber::Number((latest_blk_num - 10).into())))?.try_into().unwrap();
    println!("Balance of address {:?} is {:?} wei", address, balance);

    // eth_getBlockByNumber
    let block: eth1::Block = task::block_on(eth_api.get_block_by_number(eth1::BlockNumber::Latest, false))?;
    println!("Block => {:?}", block);
    Ok(())
}
