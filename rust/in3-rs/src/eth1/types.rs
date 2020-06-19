//! Ethereum types' definitions.
use ethereum_types::{Address, Bloom, H256, U256, U64};
use serde::{Deserialize, Serialize, Serializer};

use crate::types::Bytes;

/// Hash types representing 32-bytes hashes.
pub type Hash = H256;

/// The block number type.
pub enum BlockNumber {
    /// 32-byte number
    Number(U256),
    /// Earliest block
    Earliest,
    /// Latest block
    Latest,
    /// Pending block
    Pending,
}

impl Serialize for BlockNumber {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        match *self {
            BlockNumber::Number(ref number) => number.serialize(serializer),
            BlockNumber::Earliest => "earliest".serialize(serializer),
            BlockNumber::Latest => "latest".serialize(serializer),
            BlockNumber::Pending => "pending".serialize(serializer),
        }
    }
}

/// The transactions in a block.
#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum BlockTransactions {
    /// A vector of hashes
    Hashes(Vec<Hash>),
    /// A vector of [`Transaction`](struct.Transaction.html) objects
    Full(Vec<Transaction>),
}

/// An Ethereum transaction.
#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Transaction {
    /// Hash of the block where this transaction was in
    pub block_hash: Option<Hash>,
    /// Block number where this transaction was in
    pub block_number: Option<U256>,
    /// Address of sender
    pub from: Address,
    /// Gas
    pub gas: U256,
    /// Gas price
    pub gas_price: U256,
    /// Transaction hash
    pub hash: Hash,
    /// Data
    pub input: Bytes,
    /// Number of transactions made by the sender prior to this one
    pub nonce: U256,
    /// Address of receiver
    pub to: Option<Address>,
    /// Transaction's index position in the block
    pub transaction_index: Option<U256>,
    /// Value transferred in Wei
    pub value: U256,
    /// ECDSA recovery id
    pub v: U256,
    /// ECDSA signature r
    pub r: U256,
    /// ECDSA signature s
    pub s: U256,
}

/// An Ethereum Block.
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Block {
    /// Block number
    pub number: Option<U256>,
    /// Hash of the block
    pub hash: Option<Hash>,
    /// Hash of the parent block
    pub parent_hash: Hash,
    /// Hash of the generated proof-of-work
    pub nonce: Option<U64>,
    /// SHA3 of the uncles data in the block
    pub sha3_uncles: Hash,
    /// Bloom filter for the logs of the block
    pub logs_bloom: Option<Bloom>,
    /// Root of the transaction trie of the block
    pub transactions_root: Hash,
    /// Root of the final state trie of the block
    pub state_root: Hash,
    /// Root of the receipts trie of the block
    pub receipts_root: Hash,
    /// Address of the beneficiary to whom the mining rewards were given
    pub author: Option<Address>,
    /// Alias of `author`
    pub miner: Address,
    /// Difficulty for this block
    pub difficulty: U256,
    /// Total difficulty of the chain until this block
    pub total_difficulty: Option<U256>,
    /// 'extra data' field of this block
    pub extra_data: Bytes,
    /// Size of this block in bytes
    pub size: Option<U256>,
    /// Maximum gas allowed in this block
    pub gas_limit: U256,
    /// Total used gas by all transactions in this block
    pub gas_used: U256,
    /// unix timestamp for when the block was collated
    pub timestamp: U256,
    /// Seal fields
    pub seal_fields: Option<Vec<Bytes>>,
    /// Transactions
    pub transactions: BlockTransactions,
    /// Vector of uncle hashes
    pub uncles: Vec<Hash>,
}

/// An ethereum log.
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Log {
    /// Address from which this log originated
    pub address: Address,
    /// Hash of the block where this log was in
    pub block_hash: Option<Hash>,
    /// Block number where this log was in
    pub block_number: Option<U256>,
    /// Non-indexed arguments of the log
    pub data: Bytes,
    /// Log's index position in the block
    pub log_index: Option<U256>,
    /// Indicates whether the log was removed due to a chain reorganization
    pub removed: bool,
    /// Vector of 0 to 4 32 Bytes DATA of indexed log arguments
    pub topics: Vec<Hash>,
    /// Hash of the transaction this log was created from
    pub transaction_hash: Option<Hash>,
    /// Transaction's index position this log was created from
    pub transaction_index: Option<U256>,
    /// Log's index position in Transaction
    pub transaction_log_index: Option<U256>,
    /// Log type
    #[serde(rename = "type")]
    pub log_type: String,
}

/// Response of [`eth_getFilterChanges`](../api/struct.Api.html#method.get_filter_changes),
/// [`eth_getFilterLogs`](../api/struct.Api.html#method.get_filter_logs) &
/// [`eth_getLogs`](../api/struct.Api.html#method.get_logs) API methods.
#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum FilterChanges {
    /// A vector of [`Log`](struct.Log.html) objects
    Logs(Vec<Log>),
    /// A vector of [`Hash`](type.Hash.html)s
    BlockHashes(Vec<Hash>),
}

/// Transaction type used as i/p for
/// [`eth_sendTransaction`](../api/struct.Api.html#method.send_transaction) API method.
#[derive(Debug, Serialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct OutgoingTransaction {
    /// Address of sender
    pub from: Address,
    /// Address of receiver
    pub to: Address,
    /// Gas
    #[serde(skip_serializing_if = "Option::is_none")]
    pub gas: Option<U256>,
    /// Gas price
    #[serde(skip_serializing_if = "Option::is_none")]
    pub gas_price: Option<U256>,
    /// Value to transfer in Wei
    #[serde(skip_serializing_if = "Option::is_none")]
    pub value: Option<U256>,
    /// Data
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<Bytes>,
    /// Number of transactions made by the sender prior to this one
    #[serde(skip_serializing_if = "Option::is_none")]
    pub nonce: Option<U256>,
}

/// Transaction type used as i/p for [`eth_call`](../api/struct.Api.html#method.call) &
/// [`eth_estimateGas`](../api/struct.Api.html#method.estimate_gas) API methods.
#[derive(Debug, Serialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct CallTransaction {
    /// Address of sender
    #[serde(skip_serializing_if = "Option::is_none")]
    pub from: Option<Address>,
    /// Address of receiver (optional only for [`eth_estimateGas`](../api/struct.Api.html#method.estimate_gas)
    /// API method)
    #[serde(skip_serializing_if = "Option::is_none")]
    pub to: Option<Address>,
    /// Gas
    #[serde(skip_serializing_if = "Option::is_none")]
    pub gas: Option<U256>,
    /// Gas price
    #[serde(skip_serializing_if = "Option::is_none")]
    pub gas_price: Option<U256>,
    /// Value transferred in Wei
    #[serde(skip_serializing_if = "Option::is_none")]
    pub value: Option<U256>,
    /// Data
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<Bytes>,
}

/// An Ethereum Transaction receipt.
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TransactionReceipt {
    /// Hash of the transaction
    pub transaction_hash: Option<H256>,
    /// Transaction's index position in the block
    pub transaction_index: Option<U256>,
    /// Hash of the block where this transaction was in
    pub block_hash: Option<H256>,
    /// Block number where this transaction was in
    pub block_number: Option<U256>,
    /// Total amount of gas used when this transaction was executed in the block
    pub cumulative_gas_used: U256,
    /// Amount of gas used by this specific transaction alone
    pub gas_used: Option<U256>,
    /// Contract address created, if the transaction was a contract creation
    pub contract_address: Option<Address>,
    /// Vector of [`Log`](struct.Log.html) objects
    pub logs: Vec<Log>,
    /// Bloom filter for the logs of the block
    pub logs_bloom: Bloom,
    /// Indicates success or failure
    pub status: Option<bool>,
}
