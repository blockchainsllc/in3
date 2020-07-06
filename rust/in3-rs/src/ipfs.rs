//! IPFS JSON RPC client API.
use crate::json_rpc::json::*;
use base64::{decode, encode, DecodeError};

use crate::error::{Error, In3Result};
use crate::in3::chain::{IPFS, MULTICHAIN};
use crate::json_rpc::{rpc, Request};
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

/// IPFS [Multihash](https://github.com/multiformats/multihash) type.
pub type Multihash = String;

/// Primary interface for the IPFS JSON RPC API.
pub struct Api {
    client: Box<dyn ClientTrait>,
}

impl ApiTrait for Api {
    /// Creates an [`ipfs::Api`](../ipfs/struct.Api.html) instance by consuming a
    /// [`Client`](../in3/struct.Client.html).
    fn new(client: Box<dyn ClientTrait>) -> Self {
        assert!(client.id() == IPFS || client.id() == MULTICHAIN);
        Api { client }
    }

    /// Get a mutable reference to an [`ipfs::Api`](../ipfs/struct.Api.html)'s associated
    /// [`Client`](../in3/struct.Client.html).
    fn client(&mut self) -> &mut Box<dyn ClientTrait> {
        &mut self.client
    }
}

impl Api {
    /// Stores specified content on IPFS and returns its multihash.
    ///
    /// # Arguments
    /// * `content` - content to store on IPFS.
    pub async fn put(&mut self, content: Bytes) -> In3Result<Multihash> {
        rpc(
            self.client(),
            Request {
                method: "ipfs_put",
                params: json!([encode(content.0), "base64"]),
            },
        )
        .await
    }

    /// Returns the IPFS content associated with specified multihash.
    ///
    /// # Arguments
    /// * `hash` - multihash of content to be retrieved.
    pub async fn get(&mut self, hash: Multihash) -> In3Result<Bytes> {
        Ok(decode(
            rpc::<String>(
                self.client(),
                Request {
                    method: "ipfs_get",
                    params: json!([hash, "base64"]),
                },
            )
            .await?,
        )?
        .into())
    }
}

impl From<base64::DecodeError> for Error {
    fn from(err: DecodeError) -> Self {
        Error::Base64Error(err)
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
        let hash = task::block_on(api.put("Lorem ipsum dolor sit amet".as_bytes().into()))
            .expect("IPFS put failed");
        Ok(assert_eq!(
            hash,
            "QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8"
        ))
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
        let data =
            task::block_on(api.get("QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8".to_string()))
                .expect("IPFS get failed");
        Ok(assert_eq!(
            data,
            "Lorem ipsum dolor sit amet".as_bytes().into()
        ))
    }
}
