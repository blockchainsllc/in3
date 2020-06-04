//! Bitcoin JSON RPC client API.
use ethereum_types::U256;
use serde::Deserialize;
use serde_json::json;

use crate::error::In3Result;
use crate::eth1::Hash;
use crate::json_rpc::{Request, rpc};
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

#[derive(Debug, Deserialize)]
pub struct TransactionInput {
    vout: u32,
    txid: U256,
    sequence: u32,
    script: Bytes,
    txinwitness: Bytes,
}

#[derive(Debug, Deserialize)]
pub struct TransactionOutput {
    value: u64,
    n: u32,
    script_pubkey: Bytes,
}

#[derive(Debug, Deserialize)]
pub struct Transaction {
    in_active_chain: bool,
    data: Bytes,
    txid: U256,
    hash: Hash,
    size: u32,
    vsize: u32,
    weight: u32,
    version: u32,
    locktime: u32,
    vin: Vec<TransactionInput>,
    vout: Vec<TransactionOutput>,
    blockhash: Hash,
    confirmations: u32,
    time: u32,
    blocktime: u32,
}

#[derive(Debug, Deserialize)]
pub struct BlockHeader {
    // hash: Hash,
    confirmations: u32,
    height: u32,
    version: u32,
    // merkleroot: Hash,
    time: u32,
    nonce: u32,
    // bits: [u8; 4],
    // chainwork: U256,
    #[serde(rename = "nTx")]
    n_tx: u32,
    // #[serde(rename = "previousblockhash")]
    // previous_hash: Hash,
    // #[serde(rename = "nextblockhash")]
    // next_hash: Hash,
    #[serde(skip)]
    data: Bytes,
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum BlockTransactions {
    Hashes(Vec<Hash>),
    Transactions(Vec<Transaction>),
}

#[derive(Debug, Deserialize)]
pub struct Block {
    header: BlockHeader,
    transactions: BlockTransactions,
}


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
    pub async fn get_blockheader(&mut self, blockhash: Hash) -> In3Result<BlockHeader> {
        let hash = json!(blockhash);
        let hash_str = hash.as_str().unwrap();
        rpc(self.client(), Request {
            method: "getblockheader",
            params: json!([hash_str.trim_start_matches("0x"), true]),
        }).await
    }
}