use std::{convert, fmt};
use std::convert::TryInto;

use hex::FromHex;
use serde::{Deserialize, Serialize, Serializer};
use serde::export::Formatter;

#[derive(Debug, Serialize)]
pub struct U160(pub [u8; 20]);

// impl FromHex for U256 {
//     type Error = hex::FromHexError;
//
//     fn from_hex<T: AsRef<[u8]>>(hex: T) -> Result<Self, Self::Error> {
//         let hex = hex.as_ref();
//         let mut h = hex;
//         if hex[0] == b'0' && hex[1] == b'x' {
//             h = &hex[2..];
//         }
//         let mut h = Vec::from(h);
//         let hpadded = vec![b'0'; 64 - h.len()];
//         h.extend(hpadded);
//         println!("{:?}: {:?}", h, h.len());
//         let val = <[u8; 32]>::from_hex(h)?;
//         Ok(U256(val))
//     }
// }

#[derive(Debug, Serialize)]
pub struct U256(pub [u8; 32]);

impl convert::TryInto<u64> for U256 {
    type Error = ();

    fn try_into(self) -> Result<u64, Self::Error> {
        for n in self.0[..24].iter() {
            if *n != 0 { return Err(()); }
        }
        Ok(((self.0[24] as u64) << 56) |
            ((self.0[25] as u64) << 48) |
            ((self.0[26] as u64) << 40) |
            ((self.0[27] as u64) << 32) |
            ((self.0[28] as u64) << 24) |
            ((self.0[29] as u64) << 16) |
            ((self.0[30] as u64) << 8) |
            ((self.0[31] as u64) << 0))
    }
}

// impl convert::Into<u64> for U256 {
//     fn into(self) -> u64 {
//         ((self.0[0] as u64) << 56) |
//             ((self.0[1] as u64) << 48) |
//             ((self.0[2] as u64) << 40) |
//             ((self.0[3] as u64) << 32) |
//             ((self.0[4] as u64) << 24) |
//             ((self.0[5] as u64) << 16) |
//             ((self.0[6] as u64) << 8) |
//             ((self.0[7] as u64) << 0)
//     }
// }

// #[derive(Serialize)]
// #[serde(with = "BigArray")]
// pub struct H2048(pub [u8; 256]);
//
// impl fmt::Debug for H2048 {
//     fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
//         self.0[..].fmt(f)
//     }
// }


type Hash = U256;
type Address = U160;
type Bytes = Vec<u8>;

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
