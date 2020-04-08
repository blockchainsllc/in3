use std::i64;

use serde_json::{Result, Value};
use serde_json::json;

use crate::error::*;
use crate::error::In3Result;
use crate::in3::*;

pub struct EthApi {
    client: Box<Client>,
}

impl EthApi {
    pub fn new(config_str: &str) -> EthApi {
        let mut client = Client::new(chain::MAINNET);
        let _ = client.configure(config_str);
        EthApi { client }
    }

    async fn send(mut self, params: &str) -> In3Result<String> {
        self.client.send_request(params).await
    }

    pub async fn block_number(self) -> i64 {
        let response =
            self.send(r#"{"method": "eth_blockNumber", "params": []}"#).await;
        let v: Value = serde_json::from_str(&response.unwrap()).unwrap();
        let ret = v[0]["result"].as_str().unwrap();
        let without_prefix = ret.trim_start_matches("0x");
        let blocknum = i64::from_str_radix(without_prefix, 16);
        blocknum.unwrap_or(-1)
    }

    pub async fn getBalance(self, address: String) -> String {
        let payload = json!({
            "method": "eth_getBalance",
            "params": [
                address,
                "latest"
            ]
        });
        let serialized = serde_json::to_string(&payload).unwrap();
        let response = self.send(&serialized).await;
        let v: Value = serde_json::from_str(&response.unwrap()).unwrap();
        let balance = v[0]["result"].as_str().unwrap();
        balance.to_string()
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;

    #[test]
    fn test_block_number() {
        let api = EthApi::new(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        //execute the call to the api on task::block_on
        let num = task::block_on(api.block_number());
        assert!(num > 9000000, "Block number is not correct");
    }

    #[test]
    fn test_get_balance() {
        let api = EthApi::new(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        //execute the call to the api on task::block_on
        let num = task::block_on(
            api.getBalance("0xc94770007dda54cF92009BFF0dE90c06F603a09f".to_string()),
        );
        assert!(num != "", "Balance is not correct");
    }
}
