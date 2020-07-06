//! Bitcoin types' definitions.
use crate::eth1::Hash;
use crate::types::*;

/// The transaction in type.
#[derive(Debug)]
pub struct TransactionInput {
    /// Transaction index of output
    pub vout: u32,
    /// Transaction id of output
    pub txid: Hash,
    /// Sequence
    pub sequence: u32,
    /// Script
    pub script: Bytes,
    /// Witnessdata (if used)
    pub txinwitness: Bytes,
}

/// The transaction out type.
#[derive(Debug)]
pub struct TransactionOutput {
    /// Value of the transaction
    pub value: u64,
    /// Index
    pub n: u32,
    /// Script pubkey (or signature)
    pub script_pubkey: Bytes,
}

/// The transaction type.
#[derive(Debug)]
pub struct Transaction {
    /// True if transaction is part of the active chain
    pub in_active_chain: bool,
    /// Serialized transaction data
    pub data: Bytes,
    /// Transaction id
    pub txid: Hash,
    /// Transaction hash
    pub hash: Hash,
    /// Raw size of transaction
    pub size: u32,
    /// Virtual size of transaction
    pub vsize: u32,
    /// Weight of transaction
    pub weight: u32,
    /// Used version
    pub version: u32,
    /// Locktime
    pub locktime: u32,
    /// Vector of [`TransactionInput`](struct.TransactionInput.html) objects
    pub vin: Vec<TransactionInput>,
    /// Vector of [`TransactionOutput`](struct.TransactionOutput.html) objects
    pub vout: Vec<TransactionOutput>,
    /// Hash of block containing the transaction
    pub blockhash: Hash,
    /// Number of confirmations or blocks mined on top of the containing block
    pub confirmations: u32,
    /// Unix timestamp in seconds since 1970
    pub time: u32,
    /// Unix timestamp in seconds since 1970
    pub blocktime: u32,
}

impl From<*const in3_sys::btc_transaction> for Transaction {
    fn from(c_tx: *const in3_sys::btc_transaction) -> Self {
        unsafe {
            let mut vin = vec![];
            for i in 0..(*c_tx).vin_len {
                let tx_in = (*c_tx).vin.offset(i as isize);
                vin.push(TransactionInput {
                    vout: (*tx_in).vout,
                    txid: Hash::from_slice(&(*tx_in).txid),
                    sequence: (*tx_in).sequence,
                    script: (*tx_in).script.into(),
                    txinwitness: (*tx_in).txinwitness.into(),
                })
            }

            let mut vout = vec![];
            for i in 0..(*c_tx).vout_len {
                let tx_out = (*c_tx).vout.offset(i as isize);
                vout.push(TransactionOutput {
                    value: (*tx_out).value,
                    n: (*tx_out).n,
                    script_pubkey: (*tx_out).script_pubkey.into(),
                })
            }

            Transaction {
                in_active_chain: (*c_tx).in_active_chain,
                data: (*c_tx).data.into(),
                txid: Hash::from_slice(&(*c_tx).txid),
                hash: Hash::from_slice(&(*c_tx).hash),
                size: (*c_tx).size,
                vsize: (*c_tx).vsize,
                weight: (*c_tx).weight,
                version: (*c_tx).version,
                locktime: (*c_tx).locktime,
                vin,
                vout,
                blockhash: Hash::from_slice(&(*c_tx).blockhash),
                confirmations: (*c_tx).confirmations,
                time: (*c_tx).time,
                blocktime: (*c_tx).blocktime,
            }
        }
    }
}

/// The block header type.
#[allow(dead_code)]
pub struct BlockHeader {
    /// Hash of blockheader
    pub hash: Hash,
    /// Number of confirmations or blocks mined on top of the containing block
    pub confirmations: u32,
    /// Block number
    pub height: u32,
    /// Used version
    pub version: u32,
    /// Merkle root of the trie of all transactions in the block
    pub merkleroot: Hash,
    /// Unix timestamp in seconds since 1970
    pub time: u32,
    /// nonce-field of the block
    pub nonce: u32,
    /// bits (target) for the block
    pub bits: [u8; 4],
    /// Total amount of work since genesis
    pub chainwork: U256,
    /// Number of transactions in the block
    pub n_tx: u32,
    /// Hash of parent blockheader
    pub previous_hash: Hash,
    /// Hash of next blockheader
    pub next_hash: Hash,
    /// Raw serialized header-bytes
    pub data: [u8; 80],
}

impl From<*const in3_sys::btc_blockheader> for BlockHeader {
    fn from(c_header: *const in3_sys::btc_blockheader) -> Self {
        unsafe {
            BlockHeader {
                hash: Hash::from_slice(&(*c_header).hash),
                confirmations: (*c_header).confirmations,
                height: (*c_header).height,
                version: (*c_header).version,
                merkleroot: Hash::from_slice(&(*c_header).merkleroot),
                time: (*c_header).time,
                nonce: (*c_header).time,
                bits: (*c_header).bits.into(),
                chainwork: (*c_header).chainwork.into(),
                n_tx: (*c_header).n_tx,
                previous_hash: Hash::from_slice(&(*c_header).previous_hash),
                next_hash: Hash::from_slice(&(*c_header).next_hash),
                data: (*c_header).data.into(),
            }
        }
    }
}

impl From<in3_sys::btc_blockheader> for BlockHeader {
    fn from(header: in3_sys::btc_blockheader) -> Self {
        BlockHeader::from(&header as *const in3_sys::btc_blockheader)
    }
}

/// A block with all transactions including their full data.
#[allow(dead_code)]
pub struct BlockTransactionData {
    /// The [`BlockHeader`](struct.BlockHeader.html) of this block
    pub header: BlockHeader,
    /// Vector of [`Transaction`](struct.Transaction.html) objects
    pub transactions: Vec<Transaction>,
}

impl From<*const in3_sys::btc_block_txdata> for BlockTransactionData {
    fn from(c_blk_data: *const in3_sys::btc_block_txdata) -> Self {
        unsafe {
            let mut txs: Vec<Transaction> = vec![];
            for i in 0..(*c_blk_data).tx_len {
                let tx = (*c_blk_data).tx.offset(i as isize) as *const in3_sys::btc_transaction;
                txs.push(tx.into())
            }

            BlockTransactionData {
                header: (*c_blk_data).header.into(),
                transactions: txs,
            }
        }
    }
}

/// A block with all transaction ids.
#[allow(dead_code)]
pub struct BlockTransactionIds {
    /// The [`BlockHeader`](struct.BlockHeader.html) of this block
    pub header: BlockHeader,
    /// Vector of transaction ids
    pub transactions: Vec<Hash>,
}

impl From<*const in3_sys::btc_block_txids> for BlockTransactionIds {
    fn from(c_blk_ids: *const in3_sys::btc_block_txids) -> Self {
        unsafe {
            let mut txs: Vec<Hash> = vec![];
            for i in 0..(*c_blk_ids).tx_len {
                let tx = (*c_blk_ids).tx.offset(i as isize);
                txs.push(Hash::from_slice(&*tx))
            }

            BlockTransactionIds {
                header: (*c_blk_ids).header.into(),
                transactions: txs,
            }
        }
    }
}
