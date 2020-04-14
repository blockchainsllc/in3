use std::borrow::BorrowMut;
use std::convert::TryInto;
use std::i64;

use ethereum_types::{Address, H256, U256};
use hex::FromHex;
use serde::{Deserialize, Serialize};
use serde_json::{Result, Value};
use serde_json::json;

use crate::error::*;
use crate::eth1::{Block, BlockNumber, BlockTransactions, Hash, Transaction};
use crate::eth1::BlockTransactions::{Full, Hashes};
use crate::in3::*;

#[derive(Serialize)]
pub struct RpcRequest<'a> {
    method: &'a str,
    params: serde_json::Value,
}

pub struct Api {
    pub client: Box<Client>,
}

impl Api {
    pub fn new(client: Box<Client>) -> Api {
        Api { client }
    }

    async fn send(&mut self, params: RpcRequest<'_>) -> In3Result<serde_json::Value> {
        let req_str = serde_json::to_string(&params)?;
        let resp_str = self.client.send_request(req_str.as_str()).await?;
        let resp: serde_json::Value = serde_json::from_str(resp_str.as_str())?;
        Ok(resp)
    }

    pub async fn block_number(&mut self) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_blockNumber",
            params: json!([]),
        }).await?;
        let u256: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(u256)
    }

    pub async fn get_balance(&mut self, address: Address, block: BlockNumber) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_getBalance",
            params: json!([address, block]),
        }).await?;
        let u256: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(u256)
    }

    pub async fn get_block_by_number(&mut self, block: BlockNumber, include_tx: bool) -> In3Result<Block> {
        let resp = self.send(RpcRequest {
            method: "eth_getBlockByNumber",
            params: json!([block, include_tx]),
        }).await?;
        let mut block: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        if include_tx {
            let txs: Vec<Transaction> = serde_json::from_str(resp[0]["result"]["transactions"].to_string().as_str())?;
            block.transactions = Full(txs);
        } else {
            let txs: Vec<Hash> = serde_json::from_str(resp[0]["result"]["transactions"].to_string().as_str())?;
            block.transactions = Hashes(txs);
        }
        Ok(block)
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;

    #[test]
    fn test_block_number() {
        let mut api = Api::new(Client::new(chain::MAINNET));
        api.client.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        let num: u64 = task::block_on(api.block_number()).unwrap().try_into().unwrap();
        println!("{:?}", num);
        assert!(num > 9000000, "Block number is not correct");
    }
}
