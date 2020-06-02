use ethereum_types::{Address, U256};
use serde::Serialize;
use serde_json::json;

use crate::error::*;
use crate::eth1::{
    Block, BlockNumber, CallTransaction, FilterChanges, Hash, Log, OutgoingTransaction,
    Transaction, TransactionReceipt,
};
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

#[derive(Serialize)]
pub struct RpcRequest<'a> {
    pub method: &'a str,
    pub params: serde_json::Value,
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
    async fn send(&mut self, params: RpcRequest<'_>) -> In3Result<serde_json::Value> {
        let req_str = serde_json::to_string(&params)?;
        let resp_str = self.client.rpc(req_str.as_str()).await?;
        let resp: serde_json::Value = serde_json::from_str(resp_str.as_str())?;
        Ok(resp)
    }

    pub async fn get_storage_at(
        &mut self,
        address: Address,
        key: U256,
        block: BlockNumber,
    ) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getStorageAt",
                params: json!([address, key, block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_code(&mut self, address: Address, block: BlockNumber) -> In3Result<Bytes> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getCode",
                params: json!([address, block]),
            })
            .await?;
        let res: Bytes = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_balance(&mut self, address: Address, block: BlockNumber) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getBalance",
                params: json!([address, block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn block_number(&mut self) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_blockNumber",
                params: json!([]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn gas_price(&mut self) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_gasPrice",
                params: json!([]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_by_number(
        &mut self,
        block: BlockNumber,
        include_tx: bool,
    ) -> In3Result<Block> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getBlockByNumber",
                params: json!([block, include_tx]),
            })
            .await?;
        let res: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_by_hash(&mut self, hash: Hash, include_tx: bool) -> In3Result<Block> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getBlockByHash",
                params: json!([hash, include_tx]),
            })
            .await?;
        let res: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_logs(&mut self, filter_options: serde_json::Value) -> In3Result<Vec<Log>> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getLogs",
                params: json!([filter_options]),
            })
            .await?;
        let res: Vec<Log> = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn new_filter(&mut self, filter_options: serde_json::Value) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_newFilter",
                params: json!([filter_options]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn new_block_filter(&mut self) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_newBlockFilter",
                params: json!([]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn new_pending_transaction_filter(&mut self) -> In3Result<U256> {
        unimplemented!()
    }

    pub async fn uninstall_filter(&mut self, filter_id: U256) -> In3Result<bool> {
        let resp = self
            .send(RpcRequest {
                method: "eth_uninstallFilter",
                params: json!([filter_id]),
            })
            .await?;
        let res: bool = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_filter_changes(&mut self, filter_id: U256) -> In3Result<FilterChanges> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getFilterChanges",
                params: json!([filter_id]),
            })
            .await?;
        let res: FilterChanges = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_filter_logs(&mut self, filter_id: U256) -> In3Result<Vec<Log>> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getFilterLogs",
                params: json!([filter_id]),
            })
            .await?;
        let res: Vec<Log> = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn chain_id(&mut self) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_chainId",
                params: json!([]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_transaction_count_by_hash(&mut self, hash: Hash) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getBlockTransactionCountByHash",
                params: json!([hash]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_block_transaction_count_by_number(
        &mut self,
        block: BlockNumber,
    ) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getBlockTransactionCountByNumber",
                params: json!([block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn call(
        &mut self,
        transaction: CallTransaction,
        block: BlockNumber,
    ) -> In3Result<Bytes> {
        assert!(transaction.to.is_some());
        let resp = self
            .send(RpcRequest {
                method: "eth_call",
                params: json!([transaction, block]),
            })
            .await?;
        let res: Bytes = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn estimate_gas(
        &mut self,
        transaction: CallTransaction,
        block: BlockNumber,
    ) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_estimateGas",
                params: json!([transaction, block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_transaction_by_hash(&mut self, hash: Hash) -> In3Result<Transaction> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getTransactionByHash",
                params: json!([hash]),
            })
            .await?;
        let res: Transaction = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_transaction_by_block_hash_and_index(
        &mut self,
        hash: Hash,
        index: U256,
    ) -> In3Result<Transaction> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getTransactionByBlockHashAndIndex",
                params: json!([hash, index]),
            })
            .await?;
        let res: Transaction = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_transaction_by_block_number_and_index(
        &mut self,
        block: BlockNumber,
        index: U256,
    ) -> In3Result<Transaction> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getTransactionByBlockNumberAndIndex",
                params: json!([block, index]),
            })
            .await?;
        let res: Transaction = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_transaction_count(&mut self, block: BlockNumber) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getTransactionCount",
                params: json!([block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_uncle_by_block_number_and_index(
        &mut self,
        block: BlockNumber,
        index: U256,
    ) -> In3Result<Block> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getUncleByBlockNumberAndIndex",
                params: json!([block, index]),
            })
            .await?;
        let res: Block = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_uncle_count_by_block_hash(&mut self, hash: Hash) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getUncleCountByBlockHash",
                params: json!([hash]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_uncle_count_by_block_number(&mut self, block: BlockNumber) -> In3Result<U256> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getUncleCountByBlockNumber",
                params: json!([block]),
            })
            .await?;
        let res: U256 = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn send_transaction(&mut self, transaction: OutgoingTransaction) -> In3Result<Hash> {
        let resp = self
            .send(RpcRequest {
                method: "eth_sendTransaction",
                params: json!([transaction]),
            })
            .await?;
        let res: Hash = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn send_raw_transaction(&mut self, data: Bytes) -> In3Result<Hash> {
        let resp = self
            .send(RpcRequest {
                method: "eth_sendRawTransaction",
                params: json!([data]),
            })
            .await?;
        let res: Hash = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }

    pub async fn get_transaction_receipt(
        &mut self,
        transaction_hash: Hash,
    ) -> In3Result<TransactionReceipt> {
        let resp = self
            .send(RpcRequest {
                method: "eth_getTransactionReceipt",
                params: json!([transaction_hash]),
            })
            .await?;
        let res: TransactionReceipt = serde_json::from_str(resp[0]["result"].to_string().as_str())?;
        Ok(res)
    }
}

#[cfg(test)]
mod tests {
    use std::convert::TryInto;

    use async_std::task;

    use async_trait::async_trait;

    use serde::Serialize;
    
    use serde_json::json;


    use ethereum_types::{Address, U256};

    use crate::prelude::*;

    use super::*;

    struct MockTransport<'a> {
        responses: Vec<(&'a str, &'a str)>,
    }

    #[async_trait]
    impl Transport for MockTransport<'_> {
        async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
            let response = self.responses.pop();
            let request: serde_json::Value = serde_json::from_str(request).unwrap();
            match response {
                Some(response) if response.0 == request[0]["method"] => {
                    vec![Ok(response.1.to_string())]
                }
                _ => vec![Err(format!(
                    "Found wrong/no response while expecting response for {}",
                    request
                ))],
            }
        }

        #[cfg(feature = "blocking")]
        fn fetch_blocking(
            &mut self,
            _request: &str,
            _uris: &[&str],
        ) -> Vec<Result<String, String>> {
            unimplemented!()
        }
    }

    #[test]
    fn test_block_number() -> In3Result<()> {
        let mut api = Api::new(Client::new(chain::MAINNET));
        api.client
            .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#)?;
        api.client.set_transport(Box::new(MockTransport {
            responses: vec![(
                "eth_blockNumber",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
            )],
        }));
        let num: u64 = task::block_on(api.block_number())
            .unwrap()
            .try_into()
            .unwrap();
        Ok(assert_eq!(num, 0x96bacd))
    }

    fn init_in3() {
        let mut c = Client::new(chain::MAINNET);
        let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        c.set_transport(Box::new(MockTransport {
            responses: vec![(
                "eth_blockNumber",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
            )],
        }));
    }

    #[test]
    fn test_get_storage_at() {
        let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
        let key: U256 = 0u64.into();
        let storage: u64 = task::block_on(eth_api.get_storage_at(address, key, BlockNumber::Latest))?
            .try_into()
            .unwrap();
    }

    #[test]
    fn test_get_code() {
        let address: Address = serde_json::from_str(r#""0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f""#)?;
    let code: Bytes = task::block_on(eth_api.get_code(address, BlockNumber::Latest))?
        .try_into()
        .unwrap();
    println!("Code at address {:?} is {:?}", address, code);
        
    }

    #[test]
    fn test_get_balance() {
        let address: Address = serde_json::from_str(r#""0x0123456789012345678901234567890123456789""#)?;
        let balance: u64 = task::block_on(
            eth_api.get_balance(address, BlockNumber::Number((latest_blk_num - 10).into())),
        )?
        .try_into()
        .unwrap();
        println!("Balance of address {:?} is {:?} wei", address, balance);
    }

    #[test]
    fn test_block_number() {
        let block: Block = task::block_on(eth_api.get_block_by_number(BlockNumber::Latest, false))?;
        println!("Block => {:?}", block);
    }

    #[test]
    fn test_gas_price() {
        let gas_price: u64 = task::block_on(eth_api.gas_price())?.try_into().unwrap();
    println!("Gas price is {:?}", gas_price);
    }

    #[test]
    fn test_get_block_by_number() {
        let block: Block = task::block_on(eth_api.get_block_by_number(BlockNumber::Latest, false))?;
    println!("Block => {:?}", block);
    }

    #[test]
    fn test_get_block_by_hash() {
        // eth_getBlockByHash
    let hash: Hash = serde_json::from_str(
        r#""0xa2ad3d67e3a09d016ab72e40fc1e47d6662f9156f16ce1cce62d5805a62ffd02""#,
    )?;
    let block: Block = task::block_on(eth_api.get_block_by_hash(hash, false))?;
    println!("Block => {:?}", block);
    }

    #[test]
    fn test_get_logs() {
         // eth_getLogs
    let logs: Vec<Log> = task::block_on(eth_api.get_logs(serde_json::json!({
        "blockHash": "0x468f88ed8b40d940528552f093a11e4eb05991c787608139c931b0e9782ec5af",
        "topics": ["0xa61b5dec2abee862ab0841952bfbc161b99ad8c14738afa8ed8d5c522cd03946"]
        })))?;
        println!("Logs => {:?}", logs);
    }

    #[test]
    fn test_call() {
        let contract: Address =
        serde_json::from_str(r#""0x2736D225f85740f42D17987100dc8d58e9e16252""#).unwrap();
    let mut abi = abi::In3EthAbi::new();
    let params =
        task::block_on(abi.encode("totalServers():uint256", serde_json::json!([]))).unwrap();
    let txn = CallTransaction {
        to: Some(contract),
        data: Some(params),
        ..Default::default()
    };
    let output: Bytes = task::block_on(eth_api.call(txn, BlockNumber::Latest))
        .unwrap()
        .try_into()
        .unwrap();
    let output = task::block_on(abi.decode("uint256", output)).unwrap();
    let total_servers: U256 = serde_json::from_value(output).unwrap();
    println!("{:?}", total_servers);

    }

    #[test]
    fn test_new_filter() {
        add_response("eth_blockNumber", "[]", "\"0x84cf52\"", NULL, NULL);
        json_ctx_t* jopt = parse_json("{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}");
        size_t      fid  = eth_newFilter(in3, jopt);
        TEST_ASSERT_GREATER_THAN(0, fid);
    }

    #[test]
    fn test_new_block_filter() {
        in3_t* in3 = init_in3(mock_transport, 0x5);
  // we can add any mock json as we need trasnport but we are not calling any rpc endpoint
  //get filter id for new block
  size_t fid = eth_newBlockFilter(in3);
  TEST_ASSERT_TRUE(fid > 0);

    }

    #[test]
    fn test_uninstall_filter() {}

    #[test]
    fn test_get_filter_changes() {}

    #[test]
    fn test_get_filter_logs() {
        TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, 1, NULL));

  // Create filter options
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x1ca181\"}");
  json_ctx_t* jopt = parse_json(b);

  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);

  // Get logs
  eth_log_t *logs = NULL, *l = NULL;
  TEST_ASSERT_EQUAL(IN3_OK, eth_getFilterLogs(in3, fid, &logs));

  while (logs) {
    l    = logs;
    logs = logs->next;
    eth_log_free(l);
  }
  eth_uninstallFilter(in3, fid);
  json_free(jopt);

  // Test with non-existent filter id
  TEST_ASSERT_EQUAL(IN3_EINVAL, eth_getFilterLogs(in3, 1234, NULL));

  // Test with all filters uninstalled
  TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, fid, NULL));
    }

    #[test]
    fn test_chain_id() {}

    #[test]
    fn test_get_block_transaction_count_by_hash() {}

    #[test]
    fn test_get_block_transaction_count_by_number() {}

    
    #[test]
    fn test_estimate_gas() {}

    #[test]
    fn test_get_transaction_by_hash() {}

    #[test]
    fn test_get_transaction_by_block_hash_and_index() {}

    #[test]
    fn test_get_transaction_by_block_number_and_index() {}

    #[test]
    fn test_get_transaction_count() {}

    #[test]
    fn test_get_uncle_by_block_number_and_index() {}

    #[test]
    fn test_get_uncle_count_by_block_hash() {}

    #[test]
    fn test_get_uncle_count_by_block_number() {}

    #[test]
    fn test_send_transaction() {
        let responses = vec![
        (
            "eth_estimateGas",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x96c0"}]"#,
        ),
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
        (
            "eth_gasPrice",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x9184e72a000"}]"#,
        ),
        (
            "eth_getTransactionCount",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        ),
    ];
    eth_api.client().configure(
        r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    eth_api
        .client()
        .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    eth_api.client().set_transport(Box::new(MockTransport {
        responses: responses,
    }));
    let mut abi = abi::In3EthAbi::new();
    let params = task::block_on(abi.encode(
        "setData(uint256,string)",
        serde_json::json!([123, "testdata"]),
    ))
    .unwrap();
    println!("{:?}", params);
    let to: Address =
        serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
    let from: Address =
        serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap();
    let txn = OutgoingTransaction {
        to: to,
        from: from,
        data: Some(params),
        ..Default::default()
    };

    let hash: Hash = task::block_on(eth_api.send_transaction(txn)).unwrap();
    println!("Hash => {:?}", hash);


    }

    #[test]
    fn test_send_raw_transaction() {
        let responses = vec![
            (
                "eth_estimateGas",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x96c0"}]"#,
            ),
            (
                "eth_sendRawTransaction",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
            ),
            (
                "eth_gasPrice",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x9184e72a000"}]"#,
            ),
            (
                "eth_getTransactionCount",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
            ),
        ];
        eth_api.client().configure(
            r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
        );
        eth_api
            .client()
            .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
        eth_api.client().set_transport(Box::new(MockTransport {
            responses: responses,
        }));
        let mut abi = abi::In3EthAbi::new();
        let params = task::block_on(abi.encode(
            "setData(uint256,string)",
            serde_json::json!([123, "testdata"]),
        ))
        .unwrap();
        println!("{:?}", params);
        let to: Address =
            serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
        let from: Address =
            serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap();
        let txn = OutgoingTransaction {
            to: to,
            from: from,
            data: Some(params),
            ..Default::default()
        };
    
        let hash: Hash = task::block_on(eth_api.send_transaction(txn)).unwrap();
        println!("Hash => {:?}", hash);

    }

    #[test]
    fn test_get_transaction_receipt() {}
}
