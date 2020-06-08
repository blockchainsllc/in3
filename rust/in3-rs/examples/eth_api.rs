use std::convert::TryInto;

use async_std::task;
use ethereum_types::{Address, U256};

use in3::eth1::*;
use in3::prelude::*;
use in3::json_rpc::*;
use in3::types::Bytes;
use serde_json::Value;
fn main2() -> In3Result<()> {
    let mut transport =  MockJsonTransport{
        method: "eth_getTransactionCount"
    };
    //Make use of static string literals conversion for mock transport.
    let method = String::from(transport.method);
    let response = transport.read_json(method).to_string();
    let resp: Vec<Response> = serde_json::from_str(&response)?;
    let result = resp.first().unwrap();
    let parsed = result.to_result()?;
    let json_str:Value = serde_json::from_str(
        parsed.to_string().as_str(),
    ).unwrap();
    // let json_s = parsed.to_string().as_str();
    // let json_str = serde_json::from_str(json_s)?;
    println!("{:?}, {:?}", parsed, json_str);
    // assert_eq!(parsed.to_string().as_str(), String::from("\"0x9\""));
    Ok(())
}


fn main() -> In3Result<()> {
    let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
        method: "eth_getUncleByBlockNumberAndIndex",
    });
    let mut client = Client::new(chain::MAINNET);
    // client.set_transport(transport);
    let mut eth_api = Api::new(client);
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;
   
    // let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
    // let mut eth_api = init_api(transport, chain::MAINNET, config);
    // let number = BlockNumber::Number((1723267).into());
    let hash: Hash = serde_json::from_str(
        r#""0x685b2226cbf6e1f890211010aa192bf16f0a0cba9534264a033b023d7367b845""#,
    )?;
    let count: U256 = task::block_on(eth_api.get_uncle_count_by_block_hash(hash))?
        .try_into()
        .unwrap();
        assert!(count > (0).into());
        // assert!(blk > (0).into());
    Ok(())
}

fn main6() -> In3Result<()> {
    let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
        method: "eth_getUncleByBlockNumberAndIndex",
    });
    let mut client = Client::new(chain::MAINNET);
    // client.set_transport(transport);
    let mut eth_api = Api::new(client);
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;
   
    // let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
    // let mut eth_api = init_api(transport, chain::MAINNET, config);
    // let number = BlockNumber::Number((1723267).into());
    let number = BlockNumber::Number((56160).into());
    let block: Block =
        task::block_on(eth_api.get_uncle_by_block_number_and_index(number, (0).into()))?
            .try_into()
            .unwrap();
    let blk: U256 = block.number.unwrap();
        // assert!(blk > (0).into());
    Ok(())
}



fn main4()-> In3Result<()> {
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;
    let jopts = serde_json::json!({
        "topics": ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef"],
        "blockHash":"0x40b6019185d6ee0112445fbe678438b6968bad2e6f24ae26c7bb75461428fd43"
    });
    let fid = task::block_on(eth_api.new_filter(jopts))?;
    println!("{:?}",fid);
    // let expected: U256 = (3).into();
    Ok(())
}


fn main3() -> In3Result<()> {
    // configure client and API
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;

    // eth_getStorageAt
    let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
    let key: U256 = 0u64.into();
    let storage: u64 = task::block_on(eth_api.get_storage_at(address, key, BlockNumber::Latest))?
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
