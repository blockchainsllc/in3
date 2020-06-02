use base64::{decode, DecodeError, encode};
use serde_json::{from_str, json};

use crate::error::{Error, In3Result};
use crate::json_rpc::{Request, rpc};
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

pub type Multihash = String;

pub struct Api {
    client: Box<dyn ClientTrait>,
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
    pub async fn put(&mut self, content: Bytes) -> In3Result<Multihash> {
        let resp = rpc(self.client(), Request {
            method: "ipfs_put",
            params: json!([encode(content.0), "base64"]),
        }).await?;
        let res: Multihash = from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get(&mut self, hash: Multihash) -> In3Result<Bytes> {
        let resp = rpc(self.client(), Request {
            method: "ipfs_get",
            params: json!([hash, "base64"]),
        }).await?;
        let result: String = from_str(resp[0]["result"].to_string().as_str())?;
        let res: Bytes = decode(result)?.into();
        Ok(res)
    }
}

impl From<base64::DecodeError> for Error {
    fn from(e: DecodeError) -> Self {
        Error::CustomError(format!("Error decoding base64: {}", e))
    }
}


#[cfg(test)]
mod tests {
    use async_std::task;

    use crate::prelude::*;

    use super::*;

    #[test]
    fn test_ipfs_put() -> In3Result<()> {
        let mut api = Api::new(Client::new(chain::IPFS));
        api.client
            .configure(r#"{"autoUpdateList":false,"nodes":{"0x7d0":{"needsUpdate":false}}}}"#)?;
        api.client.set_transport(Box::new(MockTransport {
            responses: vec![(
                "ipfs_put",
                r#"[{"jsonrpc":"2.0","id":1,"result":"QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8"}]"#,
            )],
        }));
        let hash = task::block_on(
            api.put("Lorem ipsum dolor sit amet".as_bytes().into())
        ).unwrap();
        Ok(assert_eq!(hash, "QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8"))
    }

    #[test]
    fn test_ipfs_get() -> In3Result<()> {
        let mut api = Api::new(Client::new(chain::IPFS));
        api.client
            .configure(r#"{"autoUpdateList":false,"nodes":{"0x7d0":{"needsUpdate":false}}}}"#)?;
        api.client.set_transport(Box::new(MockTransport {
            responses: vec![(
                "ipfs_get",
                r#"[{"jsonrpc":"2.0","id":1,"result":"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQ="}]"#,
            )],
        }));
        let data: Bytes = task::block_on(
            api.get("QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8".to_string())
        ).unwrap();
        Ok(assert_eq!(data, "Lorem ipsum dolor sit amet".as_bytes().into()))
    }
}