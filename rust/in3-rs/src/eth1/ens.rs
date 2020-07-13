use serde::Serialize;

use async_trait::async_trait;

use crate::eth1::Hash;
use crate::json_rpc::json::*;
use crate::prelude::*;

#[derive(Serialize)]
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
}

