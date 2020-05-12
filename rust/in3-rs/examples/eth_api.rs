use std::convert::TryInto;

use async_std::task;
use ethereum_types::{Address, U256};

use in3::eth1::*;
use in3::prelude::*;
use in3::types::Bytes;

fn main() -> In3Result<()> {
    // configure client and API
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;

    // eth_getStorageAt
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let key: U256 = 0u64.into();
    let storage: u64 = task::block_on( eth_api.get_storage_at(address, key, BlockNumber::Latest))?
        .try_into()
        .unwrap();
    println!("Storage value is {:?}", storage);

    // eth_getCode
    let address: Address = serde_json::from_str(r#""0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f""#)?;
    let code: Bytes = task::block_on(eth_api.get_code(address, BlockNumber::Latest))?
        .try_into()
        .unwrap();
    println!("Code at address {:?} is {:?}", address, code);

    // eth_blockNumber
    let latest_blk_num: u64 = task::block_on(eth_api.block_number())?.try_into().unwrap();
    println!("Latest block number is {:?}", latest_blk_num);

    // eth_gasPrice
    let gas_price: u64 = task::block_on(eth_api.gas_price())?.try_into().unwrap();
    println!("Gas price is {:?}", gas_price);

    // eth_getBalance
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let balance: u64 = task::block_on(
        eth_api.get_balance(address, BlockNumber::Number((latest_blk_num - 10).into())),
    )?
    .try_into()
    .unwrap();
    println!("Balance of address {:?} is {:?} wei", address, balance);

    // eth_getBlockByNumber
    let block: Block = task::block_on(eth_api.get_block_by_number(BlockNumber::Latest, false))?;
    println!("Block => {:?}", block);

    // eth_getBlockByHash
    let hash: Hash = serde_json::from_str(
        r#""0xa2ad3d67e3a09d016ab72e40fc1e47d6662f9156f16ce1cce62d5805a62ffd02""#,
    )?;
    let block: Block = task::block_on(eth_api.get_block_by_hash(hash, false))?;
    println!("Block => {:?}", block);

    // eth_getLogs
    let logs: Vec<Log> = task::block_on(eth_api.get_logs(serde_json::json!({
    "blockHash": "0x468f88ed8b40d940528552f093a11e4eb05991c787608139c931b0e9782ec5af",
    "topics": ["0xa61b5dec2abee862ab0841952bfbc161b99ad8c14738afa8ed8d5c522cd03946"]
    })))?;
    println!("Logs => {:?}", logs);

    // eth_call
    let contract: Address =
        serde_json::from_str(r#""0x2736D225f85740f42D17987100dc8d58e9e16252""#).unwrap();
    let mut abi = abi::In3EthAbi::new();
    let params =
        task::block_on(abi.encode("totalServers():uint256", serde_json::json!([]))).unwrap();
    let txn = CallTransaction {
        to: Some(contract),
        data: Some(params),
        ..Default::default()
    };
    let output: Bytes = task::block_on(eth_api.call(txn, BlockNumber::Latest))
        .unwrap()
        .try_into()
        .unwrap();
    let output = task::block_on(abi.decode("uint256", output)).unwrap();
    let total_servers: U256 = serde_json::from_value(output).unwrap();
    println!("{:?}", total_servers);

    Ok(())
}
