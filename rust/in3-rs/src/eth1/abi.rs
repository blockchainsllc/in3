//! ABI encoder/decoder.
use async_trait::async_trait;

use crate::json_rpc::json::*;
use crate::prelude::*;

/// Trait definition for ABI encode.
#[async_trait(? Send)]
pub trait Encode {
    /// ABI encode the given parameters using given function signature as bytes.
    async fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes>;
}

#[async_trait(? Send)]
pub trait Decode {
    /// ABI decode the given bytes data using given function signature as JSON.
    async fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value>;
}

/// ABI implementation using IN3 C client's RPC.
pub struct In3EthAbi {
    in3: Box<Client>,
}

impl In3EthAbi {
    /// Create an In3EthAbi instance
    pub fn new() -> In3EthAbi {
        In3EthAbi {
            in3: Client::new(chain::LOCAL),
        }
    }
}

#[async_trait(? Send)]
impl Encode for In3EthAbi {
    /// Encode implementation using IN3's `in3_abiEncode()` RPC.
    async fn encode(&mut self, fn_sig: &str, params: Value) -> In3Result<Bytes> {
        let resp_str = self
            .in3
            .rpc(
                to_string(&json!({
                    "method": "in3_abiEncode",
                    "params": [fn_sig, params]
                }))
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = from_str(resp_str.as_str())?;
        let res: Bytes = from_str(resp["result"].to_string().as_str())?;
        Ok(res)
    }
}

#[async_trait(? Send)]
impl Decode for In3EthAbi {
    /// Decode implementation using IN3's `in3_abiDecode()` RPC.
    async fn decode(&mut self, fn_sig: &str, data: Bytes) -> In3Result<Value> {
        let resp_str = self
            .in3
            .rpc(
                to_string(&json!({"method": "in3_abiDecode", "params": [fn_sig, data]}))
                    .unwrap()
                    .as_str(),
            )
            .await?;
        let resp: Value = from_str(resp_str.as_str())?;
        Ok(resp["result"].clone())
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;
    use crate::types::Address;

    #[test]
    fn test_abi_encode() {
        let mut encoder = In3EthAbi::new();
        let address: Address = from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
        let params =
            task::block_on(encoder.encode("getBalance(address)", json!([address]))).unwrap();
        let expected: Bytes = from_str(
            r#""0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890""#,
        )
        .unwrap();
        assert_eq!(params, expected);
    }
}
