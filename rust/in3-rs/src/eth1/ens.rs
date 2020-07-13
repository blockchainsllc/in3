use serde::Serialize;

use async_trait::async_trait;

use crate::eth1::Hash;
use crate::json_rpc::json::*;
use crate::json_rpc::Error::UnexpectedResponse;
use crate::prelude::*;

#[derive(Serialize, Debug)]
#[serde(rename_all = "camelCase")]
pub enum Query {
    #[serde(rename = "addr")]
    Address,
    Resolver,
    Owner,
    Hash,
}

#[derive(Debug)]
pub enum Identifier {
    Address(Address),
    Resolver(Address),
    Owner(Address),
    Hash(Hash),
}

#[async_trait(? Send)]
pub trait Resolve {
    async fn resolve(
        &mut self,
        name: &str,
        query: Query,
        registry: Option<Address>,
    ) -> In3Result<Identifier>;

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

pub struct In3EnsResolver {
    in3: Box<Client>,
}

impl In3EnsResolver {
    pub fn new(chain: chain::ChainId) -> In3EnsResolver {
        In3EnsResolver {
            in3: Client::new(chain),
        }
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
    fn test_resolve() {
        let mut ens = In3EnsResolver::new(chain::MAINNET);
        let addrs =
            task::block_on(ens.resolve("cryptokitties.eth", Query::Resolver, None)).unwrap();
        println!("{:?}", addrs);
        // assert_eq!(params, expected);
    }
}
