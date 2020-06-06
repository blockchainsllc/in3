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
struct BlockHeaderSerdeable {
    hash: Bytes,
    confirmations: u32,
    height: u32,
    version: u32,
    merkleroot: Bytes,
    time: u32,
    nonce: u32,
    bits: Bytes,
    chainwork: Bytes,
    #[serde(rename = "nTx")]
    n_tx: u32,
    #[serde(rename = "previousblockhash")]
    previous_hash: Bytes,
    #[serde(rename = "nextblockhash")]
    next_hash: Bytes,
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


#[cfg(test)]
mod tests {
    use async_std::task;

    use crate::prelude::*;

    use super::*;

    #[test]
    fn test_btc_get_blockheader() -> In3Result<()> {
        let mut api = Api::new(Client::new(chain::BTC));
        api.client
            .configure(r#"{"autoUpdateList":false,"nodes":{"0x99":{"needsUpdate":false}}}}"#)?;
        api.client.set_transport(Box::new(MockTransport {
            responses: vec![(
                "getblockheader",
                r#"[{
                    "id": 1,
                    "jsonrpc": "2.0",
                    "result": {
                        "hash": "00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b",
                        "confirmations": 1979,
                        "height": 631076,
                        "version": 536870912,
                        "versionHex": "20000000",
                        "merkleroot": "18f5756c66b14aa8bace933001b16d78bd89a16612468122442559ce9f296eb6",
                        "time": 1589991753,
                        "mediantime": 1589989643,
                        "nonce": 2921687714,
                        "bits": "171297f6",
                        "difficulty": 15138043247082.88,
                        "chainwork": "00000000000000000000000000000000000000000f9f574f8d39680a92ad1bdc",
                        "nTx": 2339,
                        "previousblockhash": "000000000000000000061e12d6a29bd0175a6045dfffeafd950c0513f9b82c80",
                        "nextblockhash": "00000000000000000000eac6e799c468b3a140d9e1400c31f7603fdb20e1198d"
                    },
                    "in3": {
                        "proof": {
                            "cbtx": "0x010000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff470324a109fabe6d6d565bf669fbb3f2c3176649a805295685fbb0f317830d3eb7a94ab32438815a3001000000000000002865060003c9f66f712b004a9e9791052f736c7573682f00000000030f98ee31000000001976a9147c154ed1dc59609e3d26abb2df2ea3d587cd8c4188ac00000000000000002c6a4c2952534b424c4f434b3a85a796712f0d9110ed0c6c7056bf368c08f790f5e6a7a901590bd1210024303b0000000000000000266a24aa21a9ed4efc104c06ebc2d2033b0511518c2e87d6841e7716c4f37c82d54819838b491e0120000000000000000000000000000000000000000000000000000000000000000000000000",
                            "cbtxMerkleProof": "0xc8ec6a0f18df96add7d9b2bdc627cf4e3c64b5c14466494487065a360bc614eebe66ba1702aa7a0eef97146f5b0fd544dad9bad33125187bc1933853f94dd54a176baa3181b168eb6eca2dc7953d167ff5b12beec84baa6ca89b80341e329f96db85db46909d0a773570895828df2cf30122d85e54a52e8b0a96f75cfe15d75412e904251b18b31903954fdd7c609c60e5564d563853cdf0d59f4ade9e19bb3b0e6f1dda2ae3d63fc8e04892e75f989656e7c8823584cb1cd59a857bd206d684d83b599702a6078ad9bd2cdeea597445f5f02e054fcf9a049d4ee0e6c07b4ef5b07ca72a1be41bfb2b800b77255755c0fa7280582c48f7e4352a662e05dc787aefbbf1978227ce6933be11f6edafcf41b8fcc927915722f22fd9650190c46a84d966f4b79c89f4d8c858ebfc42d4431c1d8877dac16c2d9cecee6f642d84ca437c4fe68d2020d078c1b9bfcc07ad370dc85b19e59666f40ba3a5a3006e667093751032f35590137e0c77d99b6edca34b8dbc8413d4df3842d3908ae4dfb8adcf"
                        },
                        "lastNodeList": 2809569,
                        "execTime": 95,
                        "rpcTime": 0,
                        "rpcCount": 0,
                        "currentBlock": 2816454,
                        "version": "2.1.0"
                    }
                }]"#,
            )],
        }));
        let header = task::block_on(
            api.get_blockheader(serde_json::from_str::<Hash>(r#""0x00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b""#)?)
        ).unwrap();
        Ok(assert_eq!(header.height, 631076))
    }
}
