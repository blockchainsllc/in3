use ethereum_types::{Address, H256, U256};
use hex::FromHex;
use serde::{Deserialize, Serialize, Serializer};

type Bytes = Vec<u8>;
type Hash = H256;

enum BlockNumber {
    Number(U256),
    Earliest,
    Latest,
    Pending,
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
