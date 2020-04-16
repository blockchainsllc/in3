use ethereum_types::{Address, Bloom, H256, U256, U64};
use serde::{Deserialize, Serialize, Serializer};

use crate::types::Bytes;

pub type Hash = H256;

pub enum BlockNumber {
    Number(U256),
    Earliest,
    Latest,
    Pending,
}

impl Serialize for BlockNumber {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where S: Serializer {
        match *self {
            BlockNumber::Number(ref number) => number.serialize(serializer),
            BlockNumber::Earliest => "earliest".serialize(serializer),
            BlockNumber::Latest => "latest".serialize(serializer),
            BlockNumber::Pending => "pending".serialize(serializer),
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum BlockTransactions {
    Hashes(Vec<Hash>),
    Full(Vec<Transaction>),
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Transaction {
    pub block_hash: Option<Hash>,
    pub block_number: Option<U256>,
    pub from: Address,
    pub gas: U256,
    pub gas_price: U256,
    pub hash: Hash,
    pub input: Bytes,
    pub nonce: U256,
    pub to: Option<Address>,
    pub transaction_index: Option<U256>,
    pub value: U256,
    pub v: U256,
    pub r: U256,
    pub s: U256,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Block {
    pub number: Option<U256>,
    pub hash: Option<Hash>,
    pub parent_hash: Hash,
    pub nonce: Option<U64>,
    pub sha3_uncles: Hash,
    pub logs_bloom: Option<Bloom>,
    pub transactions_root: Hash,
    pub state_root: Hash,
    pub receipts_root: Hash,
    pub author: Option<Address>,
    pub miner: Address,
    pub difficulty: U256,
    pub total_difficulty: Option<U256>,
    pub extra_data: Bytes,
    pub size: Option<U256>,
    pub gas_limit: U256,
    pub gas_used: U256,
    pub timestamp: U256,
    pub seal_fields: Option<Vec<Bytes>>,
    pub transactions: BlockTransactions,
    pub uncles: Vec<Hash>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Log {
    pub address: Address,
    pub block_hash: Option<Hash>,
    pub block_number: Option<U256>,
    pub data: Bytes,
    pub log_index: Option<U256>,
    pub removed: bool,
    pub topics: Vec<Hash>,
    pub transaction_hash: Option<Hash>,
    pub transaction_index: Option<U256>,
    pub transaction_log_index: Option<U256>,
    #[serde(rename = "type")]
    pub log_type: String,
}