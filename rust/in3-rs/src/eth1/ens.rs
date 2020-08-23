//! ENS resolver.
use serde::Serialize;

use async_trait::async_trait;

use crate::eth1::Hash;
use crate::json_rpc::json::*;
use crate::json_rpc::Error::UnexpectedResponse;
use crate::prelude::*;

/// Query type for ENS resolver.
#[derive(Serialize, Debug)]
#[serde(rename_all = "camelCase")]
pub enum Query {
    #[serde(rename = "addr")]
    Address,
    Resolver,
    Owner,
    Hash,
}

/// Identifier type for ENS resolver.
#[derive(Debug, PartialEq)]
pub enum Identifier {
    Address(Address),
    Resolver(Address),
    Owner(Address),
    Hash(Hash),
}

/// Trait definition for ENS resolve.
#[async_trait(? Send)]
pub trait Resolve {
    /// Resolves the specified query for given ENS name.
    async fn resolve(
        &mut self,
        name: &str,
        query: Query,
        registry: Option<Address>,
    ) -> In3Result<Identifier>;

    /// Resolves the address for for given ENS name.
    async fn resolve_address(
        &mut self,
        name: &str,
        registry: Option<Address>,
    ) -> In3Result<Address> {
        let id = self.resolve(name, Query::Address, registry).await?;
        if let Identifier::Address(val) = id {
            Ok(val)
        } else {
            Err(UnexpectedResponse {
                actual: format!("{:?}", id),
                expected: format!("{:?}", Query::Address),
            }
            .into())
        }
    }

    /// Resolves the resolver for for given ENS name.
    async fn resolve_resolver(
        &mut self,
        name: &str,
        registry: Option<Address>,
    ) -> In3Result<Address> {
        let id = self.resolve(name, Query::Resolver, registry).await?;
        if let Identifier::Resolver(val) = id {
            Ok(val)
        } else {
            Err(UnexpectedResponse {
                actual: format!("{:?}", id),
                expected: format!("{:?}", Query::Resolver),
            }
            .into())
        }
    }

    /// Resolves the owner for for given ENS name.
    async fn resolve_owner(&mut self, name: &str, registry: Option<Address>) -> In3Result<Address> {
        let id = self.resolve(name, Query::Owner, registry).await?;
        if let Identifier::Owner(val) = id {
            Ok(val)
        } else {
            Err(UnexpectedResponse {
                actual: format!("{:?}", id),
                expected: format!("{:?}", Query::Owner),
            }
            .into())
        }
    }

    /// Resolves the hash for for given ENS name.
    async fn resolve_hash(&mut self, name: &str, registry: Option<Address>) -> In3Result<Hash> {
        let id = self.resolve(name, Query::Hash, registry).await?;
        if let Identifier::Hash(val) = id {
            Ok(val)
        } else {
            Err(UnexpectedResponse {
                actual: format!("{:?}", id),
                expected: format!("{:?}", Query::Hash),
            }
            .into())
        }
    }
}

/// ENS resolver implementation using IN3's C code.
pub struct In3EnsResolver {
    in3: Box<Client>,
}

impl In3EnsResolver {
    /// Create an In3EnsResolver instance
    pub fn new(chain: chain::ChainId) -> In3EnsResolver {
        In3EnsResolver {
            in3: Client::new(chain),
        }
    }

    #[allow(dead_code)]
    pub(crate) fn for_client(client: Box<Client>) -> In3EnsResolver {
        In3EnsResolver { in3: client }
    }
}

#[async_trait(? Send)]
impl Resolve for In3EnsResolver {
    /// Resolve implementation using IN3's `in3_ens()` RPC.
    async fn resolve(
        &mut self,
        name: &str,
        query: Query,
        registry: Option<Address>,
    ) -> In3Result<Identifier> {
        let resp_str = self
            .in3
            .rpc(
                to_string(&json!({
                    "method": "in3_ens",
                    "params": [name, query, registry]
                }))
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = from_str(resp_str.as_str())?;
        Ok(match query {
            Query::Address => Identifier::Address(from_str(resp["result"].to_string().as_str())?),
            Query::Resolver => Identifier::Resolver(from_str(resp["result"].to_string().as_str())?),
            Query::Owner => Identifier::Owner(from_str(resp["result"].to_string().as_str())?),
            Query::Hash => Identifier::Hash(from_str(resp["result"].to_string().as_str())?),
        })
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;

    #[test]
    fn test_ens_resolve() {
        let responses = vec![
            (
                "eth_call",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0xc2c8834632ffec7165e54c485f8df7c1e2382f7f1ebb0b30171a6eecdf791421"}]"#,
            ),
            (
                "eth_call",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x12e3da7ef085ef9e267defa33bec426730379efb"}]"#,
            ),
            (
                "eth_call",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x1da022710df5002339274aadee8d58218e9d6ab5"}]"#,
            ),
            (
                "eth_call",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x06012c8cf97bead5deae237070f9587f8e7a266d"}]"#,
            ),
            (
                "eth_call",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x1da022710df5002339274aadee8d58218e9d6ab5"}]"#,
            ),
        ];
        let transport: Box<dyn Transport> = Box::new(MockTransport { responses });

        let mut client = Client::new(chain::MAINNET);
        let _ = client.configure(r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"proof":"none","nodes":{"0x1":{"needsUpdate":false}}}}"#);
        client.set_transport(transport);
        let mut ens = In3EnsResolver::for_client(client);

        assert_eq!(
            Identifier::Address(
                from_str(r#""0x06012c8cf97bead5deae237070f9587f8e7a266d""#).unwrap()
            ),
            task::block_on(ens.resolve("cryptokitties.eth", Query::Address, None)).unwrap()
        );
        assert_eq!(
            from_str::<Address>(r#""0x1da022710df5002339274aadee8d58218e9d6ab5""#).unwrap(),
            task::block_on(ens.resolve_resolver("cryptokitties.eth", None)).unwrap()
        );
        assert_eq!(
            from_str::<Address>(r#""0x12e3da7ef085ef9e267defa33bec426730379efb""#).unwrap(),
            task::block_on(ens.resolve_owner("cryptokitties.eth", None)).unwrap()
        );
        assert_eq!(
            from_str::<Hash>(
                r#""0xc2c8834632ffec7165e54c485f8df7c1e2382f7f1ebb0b30171a6eecdf791421""#
            )
            .unwrap(),
            task::block_on(ens.resolve_hash("cryptokitties.eth", None)).unwrap()
        );
    }
}
