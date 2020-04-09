#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Block {
    pub hash: Option<H256>,
    pub parent_hash: H256,
    pub sha3_uncles: H256,
    pub author: H160,
    pub miner: H160,
    pub state_root: H256,
    pub transactions_root: H256,
    pub receipts_root: H256,
    pub number: Option<U256>,
    pub gas_used: U256,
    pub gas_limit: U256,
    pub extra_data: Bytes,
    pub logs_bloom: Option<H2048>,
    pub timestamp: U256,
    pub difficulty: U256,
    pub total_difficulty: Option<U256>,
    pub seal_fields: Vec<Bytes>,
    pub uncles: Vec<H256>,
    pub transactions: BlockTransactions,
    pub size: Option<U256>,
}
