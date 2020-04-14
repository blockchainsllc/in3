use std::borrow::BorrowMut;
use std::convert::TryInto;
use std::i64;

use ethereum_types::{H256, U256};
use hex::FromHex;
use serde::{Deserialize, Serialize};
use serde_json::{Result, Value};
use serde_json::json;

use crate::error::*;
use crate::in3::*;

#[derive(Serialize)]
pub struct RpcRequest<'a> {
    method: &'a str,
    params: serde_json::Value,
}

pub struct EthApi {
    pub client: Box<Client>,
}

impl EthApi {
    pub fn new(client: Box<Client>) -> EthApi {
        EthApi { client }
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

    // pub async fn getBalance(&mut self, address: String) -> String {
    //     let payload = json!({
    //         "method": "eth_getBalance",
    //         "params": [
    //             address,
    //             "latest"
    //         ]
    //     });
    //     let serialized = serde_json::to_string(&payload).unwrap();
    //     let response = self.send(&serialized).await;
    //     let v: Value = serde_json::from_str(&response.unwrap()).unwrap();
    //     let balance = v[0]["result"].as_str().unwrap();
    //     balance.to_string()
    // }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;

    #[test]
    fn test_block_number() {
        let mut api = EthApi::new(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        let num: u64 = task::block_on(api.block_number()).unwrap().try_into().unwrap();
        println!("{:?}", num);
        assert!(num > 9000000, "Block number is not correct");
    }

    // #[test]
    // fn test_get_balance() {
    //     let mut api = EthApi::new(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    //     //execute the call to the api on task::block_on
    //     let num = task::block_on(
    //         api.getBalance("0xc94770007dda54cF92009BFF0dE90c06F603a09f".to_string()),
    //     );
    //     assert!(num != "", "Balance is not correct");
    // }
}
