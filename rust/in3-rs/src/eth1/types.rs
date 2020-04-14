use std::fmt;

use ethereum_types::{Address, H256, U256, U64};
use rustc_hex::{FromHex, ToHex};
use serde::{Deserialize, Serialize, Serializer};
use serde::de::{Error, Visitor};
use serde::Deserializer;

#[derive(Debug, PartialEq, Eq, Default, Hash, Clone)]
pub struct Bytes(pub Vec<u8>);

impl Bytes {
    pub fn new(bytes: Vec<u8>) -> Bytes {
        Bytes(bytes)
    }
    pub fn into_vec(self) -> Vec<u8> {
        self.0
    }
}

impl From<Vec<u8>> for Bytes {
    fn from(bytes: Vec<u8>) -> Bytes {
        Bytes(bytes)
    }
}

impl Into<Vec<u8>> for Bytes {
    fn into(self) -> Vec<u8> {
        self.0
    }
}

impl Serialize for Bytes {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where S: Serializer
    {
        let mut serialized = "0x".to_owned();
        serialized.push_str(self.0.to_hex().as_str());
        serializer.serialize_str(serialized.as_ref())
    }
}

impl<'a> Deserialize<'a> for Bytes {
    fn deserialize<D>(deserializer: D) -> Result<Bytes, D::Error>
        where D: Deserializer<'a> {
        deserializer.deserialize_any(BytesVisitor)
    }
}

struct BytesVisitor;

impl<'a> Visitor<'a> for BytesVisitor {
    type Value = Bytes;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "a 0x-prefixed, hex-encoded vector of bytes")
    }

    fn visit_str<E>(self, value: &str) -> Result<Self::Value, E> where E: Error {
        if value.len() >= 2 && value.starts_with("0x") && value.len() & 1 == 0 {
            Ok(Bytes::new(FromHex::from_hex(&value[2..]).map_err(|e| Error::custom(format!("Invalid hex: {}", e)))?))
        } else {
            Err(Error::custom("Invalid bytes format. Expected a 0x-prefixed hex string with even length"))
        }
    }

    fn visit_string<E>(self, value: String) -> Result<Self::Value, E> where E: Error {
        self.visit_str(value.as_ref())
    }
}

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

#[derive(Debug)]
pub enum BlockTransactions {
    Hashes(Vec<Hash>),
    Full(Vec<Transaction>),
}

impl Serialize for BlockTransactions {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where S: Serializer {
        match *self {
            BlockTransactions::Hashes(ref hashes) => hashes.serialize(serializer),
            BlockTransactions::Full(ref ts) => ts.serialize(serializer)
        }
    }
}

#[derive(Debug, Serialize)]
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
// pub creates: Option<Address>,
// pub raw: Bytes,
// pub public_key: Option<H512>,
// pub chain_id: Option<u64>,
// pub standard_v: U256,
// pub condition: Option<TransactionCondition>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Block {
    pub number: Option<U256>,
    pub hash: Option<Hash>,
    pub parent_hash: Hash,
    pub nonce: Option<u64>,
    pub sha3_uncles: Hash,
    // pub logs_bloom: Option<H2048>,
    pub transactions_root: Hash,
    pub state_root: Hash,
    pub receipts_root: Hash,
    pub author: Address,
    pub miner: Address,
    pub difficulty: U256,
    pub total_difficulty: Option<U256>,
    pub extra_data: Bytes,
    pub size: Option<U256>,
    pub gas_limit: U256,
    pub gas_used: U256,
    pub timestamp: U256,
    pub seal_fields: Vec<Bytes>,
    pub transactions: BlockTransactions,
    pub uncles: Vec<Hash>,
}
