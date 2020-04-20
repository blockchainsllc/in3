use async_std::task;
use serde_json::{json, Value};

use crate::prelude::*;
use crate::types::Bytes;

pub trait Encode {
    fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes>;
}

pub trait Decode {
    fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value>;
}

pub struct In3EthAbi {
    in3: Box<Client>,
}

impl In3EthAbi {
    pub fn new() -> In3EthAbi {
        In3EthAbi { in3: Client::new(chain::LOCAL) }
    }
}

impl Encode for In3EthAbi {
    fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes> {
        let resp_str = task::block_on(
            self.in3.rpc(
                serde_json::to_string(&json!({
                    "method": "in3_abiEncode",
                    "params": [fn_sig, params]
                })).unwrap().as_str()
            ),
        )?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        let res: Bytes = serde_json::from_str(resp["result"].to_string().as_str())?;
        Ok(res)
    }
}

impl Decode for In3EthAbi {
    fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value> {
        let resp_str = task::block_on(
            self.in3.rpc(
                serde_json::to_string(&json!({
                    "method": "in3_abiDecode",
                    "params": [fn_sig, data]
                })).unwrap().as_str()
            ),
        )?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        Ok(resp["result"].clone())
    }
}

#[cfg(test)]
mod tests {
    use ethereum_types::Address;

    use crate::eth1::api::Api;

    use super::*;

    #[test]
    fn test_abi_encode() {
        let mut api = Api::new(Client::new(chain::MAINNET));
        let mut encoder = In3EthAbi::new();
        let address: Address = serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
        let params = encoder.encode("getBalance(address)", json!([address])).unwrap();
        let expected: Bytes = serde_json::from_str(r#""0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890""#).unwrap();
        assert_eq!(params, expected);
    }
}
