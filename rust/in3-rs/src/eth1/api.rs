use ethereum_types::{Address, U256};
use serde::Serialize;
use serde_json::json;

use crate::error::*;
use crate::eth1::{Block, BlockNumber, Hash, Log};
use crate::prelude::*;
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

#[derive(Serialize)]
struct RpcRequest<'a> {
    method: &'a str,
    params: serde_json::Value,
}

pub struct Api {
    client: Box<dyn ClientTrait>
}

impl ApiTrait for Api {
    fn new(client: Box<dyn ClientTrait>) -> Self {
        Api { client }
    }

    fn client(&mut self) -> &mut Box<dyn ClientTrait> {
        &mut self.client
    }
}


impl Api {
    async fn send(&mut self, params: RpcRequest<'_>) -> In3Result<serde_json::Value> {
        let req_str = serde_json::to_string(&params)?;
        let resp_str = self.client.rpc(req_str.as_str()).await?;
        let resp: serde_json::Value = serde_json::from_str(resp_str.as_str())?;
        Ok(resp)
    }

    pub async fn get_storage_at(&mut self, address: Address, key: U256, block: BlockNumber) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_getStorageAt",
            params: json!([address, key, block]),
        }).await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_code(&mut self, address: Address, block: BlockNumber) -> In3Result<Bytes> {
        let resp = self.send(RpcRequest {
            method: "eth_getCode",
            params: json!([address, block]),
        }).await?;
        let res: Bytes = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_balance(&mut self, address: Address, block: BlockNumber) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_getBalance",
            params: json!([address, block]),
        }).await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn block_number(&mut self) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_blockNumber",
            params: json!([]),
        }).await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn gas_price(&mut self) -> In3Result<U256> {
        let resp = self.send(RpcRequest {
            method: "eth_gasPrice",
            params: json!([]),
        }).await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_by_number(&mut self, block: BlockNumber, include_tx: bool) -> In3Result<Block> {
        let resp = self.send(RpcRequest {
            method: "eth_getBlockByNumber",
            params: json!([block, include_tx]),
        }).await?;
        let res: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_by_hash(&mut self, hash: Hash, include_tx: bool) -> In3Result<Block> {
        let resp = self.send(RpcRequest {
            method: "eth_getBlockByHash",
            params: json!([hash, include_tx]),
        }).await?;
        let res: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_logs(&mut self, filter_options: serde_json::Value) -> In3Result<Vec<Log>> {
        let resp = self.send(RpcRequest {
            method: "eth_getLogs",
            params: json!([filter_options]),
        }).await?;
        let res: Vec<Log> = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }
}

#[cfg(test)]
mod tests {
    use std::convert::TryInto;

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

    #[test]
    fn test_api() {
        let mut api = Api::new(Client::new(chain::MAINNET));
        api.client.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        let num: u64 = task::block_on(api.block_number()).unwrap().try_into().unwrap();
        println!("{:?}", num);
        assert!(num > 9000000, "Block number is not correct");
    }
}
