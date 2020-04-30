use serde_json::{json, Value};

use async_trait::async_trait;

use crate::prelude::*;
use crate::types::Bytes;

#[async_trait(? Send)]
pub trait Encode {
    async fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes>;
}

#[async_trait(? Send)]
pub trait Decode {
    async fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value>;
}

pub struct In3EthAbi {
    in3: Box<Client>,
}

impl In3EthAbi {
    pub fn new() -> In3EthAbi {
        In3EthAbi {
            in3: Client::new(chain::LOCAL),
        }
    }
}

#[async_trait(? Send)]
impl Encode for In3EthAbi {
    async fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes> {
        let resp_str = self
            .in3
            .rpc(
                serde_json::to_string(&json!({
                    "method": "in3_abiEncode",
                    "params": [fn_sig, params]
                }))
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        let res: Bytes = serde_json::from_str(resp["result"].to_string().as_str())?;
        Ok(res)
    }
}

#[async_trait(? Send)]
impl Decode for In3EthAbi {
    async fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value> {
        let resp_str = self
            .in3
            .rpc(
                serde_json::to_string(
                    &json!({"method": "in3_abiDecode", "params": [fn_sig, data]}),
                )
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        Ok(resp["result"].clone())
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;
    use ethereum_types::Address;

    use super::*;

    #[test]
    fn test_abi_encode() {
        let mut encoder = In3EthAbi::new();
        let address: Address =
            serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
        let params =
            task::block_on(encoder.encode("getBalance(address)", json!([address]))).unwrap();
        let expected: Bytes = serde_json::from_str(
            r#""0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890""#,
        )
        .unwrap();
        assert_eq!(params, expected);
    }
}
