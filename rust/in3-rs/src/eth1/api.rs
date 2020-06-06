//! Ethereum JSON RPC client API. This implementation is more or less consistent with the
//! [Ethereum JSON RPC wiki](https://github.com/ethereum/wiki/wiki/JSON-RPC).
use ethereum_types::{Address, U256};
use serde_json::json;

use crate::error::*;
use crate::eth1::{
    Block, BlockNumber, CallTransaction, FilterChanges, Hash, Log, OutgoingTransaction,
    Transaction, TransactionReceipt,
};
use crate::json_rpc::{rpc, Request};
use crate::traits::{Api as ApiTrait, Client as ClientTrait};
use crate::types::Bytes;

/// Primary interface for the Ethereum JSON RPC API.
pub struct Api {
    client: Box<dyn ClientTrait>,
}

impl ApiTrait for Api {
    /// Creates an [`eth1::Api`](../eth1/api/struct.Api.html) instance by consuming a
    /// [`Client`](../in3/struct.Client.html).
    fn new(client: Box<dyn ClientTrait>) -> Self {
        Api { client }
    }

    /// Get a mutable reference to an [`eth1::Api`](../eth1/api/struct.Api.html)'s associated
    /// [`Client`](../in3/struct.Client.html).
    fn client(&mut self) -> &mut Box<dyn ClientTrait> {
        &mut self.client
    }
}

impl Api {
    async fn send(&mut self, params: RpcRequest<'_>) -> In3Result<serde_json::Value> {
        let req_str = serde_json::to_string(&params)?;
        println!("{:?}", req_str.as_str());
        let resp_str = self.client.rpc(req_str.as_str()).await?;
        let resp: serde_json::Value = serde_json::from_str(resp_str.as_str())?;
        println!("{:?}", resp);
        // let resp: serde_json::Value = serde_json::from_str(&resp_str.to_string())?;
        Ok(resp)
    }

    /// Returns the value from a storage position at a given address.
    ///
    /// # Arguments
    /// * `address` - address of the storage.
    /// * `key` - position in the storage.
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_storage_at(
        &mut self,
        address: Address,
        key: U256,
        block: BlockNumber,
    ) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getStorageAt",
                params: json!([address, key, block]),
            },
        )
        .await
    }

    /// Returns code at a given address.
    ///
    /// # Arguments
    /// * `address` - address
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_code(&mut self, address: Address, block: BlockNumber) -> In3Result<Bytes> {
        rpc(
            self.client(),
            Request {
                method: "eth_getCode",
                params: json!([address, block]),
            },
        )
        .await
    }

    /// Returns the balance of the account of given address.
    ///
    /// # Arguments
    /// * `address` - address
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_balance(&mut self, address: Address, block: BlockNumber) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getBalance",
                params: json!([address, block]),
            },
        )
        .await
    }

    /// Returns the number of most recent block.
    pub async fn block_number(&mut self) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_blockNumber",
                params: json!([]),
            },
        )
        .await
    }

    /// Returns the current price per gas in wei.
    pub async fn gas_price(&mut self) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_gasPrice",
                params: json!([]),
            },
        )
        .await
    }

    /// Returns information about a block by block number.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    /// * `include_tx` - boolean which if true returns the full transaction objects otherwise only
    /// the hashes of the transactions.
    pub async fn get_block_by_number(
        &mut self,
        block: BlockNumber,
        include_tx: bool,
    ) -> In3Result<Block> {
        rpc(
            self.client(),
            Request {
                method: "eth_getBlockByNumber",
                params: json!([block, include_tx]),
            },
        )
        .await
    }

    /// Returns information about a block by hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    /// * `include_tx` - boolean which if true returns the full transaction objects otherwise only
    /// the hashes of the transactions.
    pub async fn get_block_by_hash(&mut self, hash: Hash, include_tx: bool) -> In3Result<Block> {
        rpc(
            self.client(),
            Request {
                method: "eth_getBlockByHash",
                params: json!([hash, include_tx]),
            },
        )
        .await
    }

    /// Returns an array of all logs matching a given filter object.
    ///
    /// # Arguments
    /// * `filter_options` - options serialized as a JSON object -
    ///     * fromBlock: (optional, default: "latest") Integer block number, or "latest" for the
    ///         last mined block or "pending", "earliest" for not yet mined transactions.
    ///     * toBlock: (optional, default: "latest") Integer block number, or "latest" for the last
    ///         mined block or "pending", "earliest" for not yet mined transactions.
    ///     * address: (optional) Contract address or a list of addresses from which logs should
    ///         originate.
    ///     * topics: (optional) Array of 32 Bytes DATA topics. Each topic can also be an array of
    ///         DATA with "or" options.
    pub async fn get_logs(&mut self, filter_options: serde_json::Value) -> In3Result<Vec<Log>> {
        rpc(
            self.client(),
            Request {
                method: "eth_getLogs",
                params: json!([filter_options]),
            },
        )
        .await
    }

    /// Creates a filter object, based on filter options.
    ///
    /// # Arguments
    /// * `filter_options` - options serialized as a JSON object -
    ///     * fromBlock: (optional, default: "latest") Integer block number, or "latest" for the
    ///         last mined block or "pending", "earliest" for not yet mined transactions.
    ///     * toBlock: (optional, default: "latest") Integer block number, or "latest" for the last
    ///         mined block or "pending", "earliest" for not yet mined transactions.
    ///     * address: (optional) Contract address or a list of addresses from which logs should
    ///         originate.
    ///     * topics: (optional) Array of 32 Bytes DATA topics. Each topic can also be an array of
    ///         DATA with "or" options.
    pub async fn new_filter(&mut self, filter_options: serde_json::Value) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_newFilter",
                params: json!([filter_options]),
            },
        )
        .await
    }

    /// Creates a filter in the node, to notify when a new block arrives.
    pub async fn new_block_filter(&mut self) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_newBlockFilter",
                params: json!([]),
            },
        )
        .await
    }

    /// Creates a filter in the node, to notify when new pending transactions arrive.
    ///
    /// # Panics
    /// This function is not implemented and calls to it will panic.
    pub async fn new_pending_transaction_filter(&mut self) -> In3Result<U256> {
        unimplemented!()
    }

    /// Uninstalls a filter with given id.
    ///
    /// # Arguments
    /// * `filter_id` - id of filter that needs to be uninstalled.
    pub async fn uninstall_filter(&mut self, filter_id: U256) -> In3Result<bool> {
        rpc(
            self.client(),
            Request {
                method: "eth_uninstallFilter",
                params: json!([filter_id]),
            },
        )
        .await
    }

    /// Polling method for a filter, which returns an array of logs which occurred since last poll.
    ///
    /// # Arguments
    /// * `filter_id` - id of filter that must be polled.
    pub async fn get_filter_changes(&mut self, filter_id: U256) -> In3Result<FilterChanges> {
        rpc(
            self.client(),
            Request {
                method: "eth_getFilterChanges",
                params: json!([filter_id]),
            },
        )
        .await
    }

    /// Returns an array of all logs matching filter with given id.
    ///
    /// # Arguments
    /// * `filter_id` - id of filter that must be matched against.
    pub async fn get_filter_logs(&mut self, filter_id: U256) -> In3Result<Vec<Log>> {
        rpc(
            self.client(),
            Request {
                method: "eth_getFilterLogs",
                params: json!([filter_id]),
            },
        )
        .await
    }

    /// Returns the currently configured chain id, a value used in replay-protected transaction
    /// signing as introduced by EIP-155.
    pub async fn chain_id(&mut self) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_chainId",
                params: json!([]),
            },
        )
        .await
    }

    /// Returns the number of transactions in a block from a block matching the given block hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    pub async fn get_block_transaction_count_by_hash(&mut self, hash: Hash) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getBlockTransactionCountByHash",
                params: json!([hash]),
            },
        )
        .await
    }

    /// Returns the number of transactions in a block matching the given block number.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_block_transaction_count_by_number(
        &mut self,
        block: BlockNumber,
    ) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getBlockTransactionCountByNumber",
                params: json!([block]),
            },
        )
        .await
    }

    /// Executes a new message call immediately without creating a transaction on the block chain.
    ///
    /// # Arguments
    /// * `transaction` - [`CallTransaction`](../types/struct.CallTransaction.html)
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn call(
        &mut self,
        transaction: CallTransaction,
        block: BlockNumber,
    ) -> In3Result<Bytes> {
        assert!(transaction.to.is_some());
        rpc(
            self.client(),
            Request {
                method: "eth_call",
                params: json!([transaction, block]),
            },
        )
        .await
    }

    /// Generates and returns an estimate of how much gas is necessary to allow the transaction to
    /// complete.
    ///
    /// Note that the estimate may be significantly more than the amount of gas actually used by
    /// the transaction, for a variety of reasons including EVM mechanics and node performance.
    ///
    /// # Arguments
    /// * `transaction` - [`CallTransaction`](../types/struct.CallTransaction.html)
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn estimate_gas(
        &mut self,
        transaction: CallTransaction,
        block: BlockNumber,
    ) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_estimateGas",
                params: json!([transaction, block]),
            },
        )
        .await
    }

    /// Returns the information about a transaction requested by transaction hash.
    ///
    /// # Arguments
    /// * `hash` - hash of transaction.
    pub async fn get_transaction_by_hash(&mut self, hash: Hash) -> In3Result<Transaction> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionByHash",
                params: json!([hash]),
            },
        )
        .await
    }

    /// Returns information about a transaction by block hash and transaction index position.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    /// * `index` - transaction index position.
    pub async fn get_transaction_by_block_hash_and_index(
        &mut self,
        hash: Hash,
        index: U256,
    ) -> In3Result<Transaction> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionByBlockHashAndIndex",
                params: json!([hash, index]),
            },
        )
        .await
    }

    /// Returns information about a transaction by block number and transaction index position.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    /// * `index` - transaction index position.
    pub async fn get_transaction_by_block_number_and_index(
        &mut self,
        block: BlockNumber,
        index: U256,
    ) -> In3Result<Transaction> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionByBlockNumberAndIndex",
                params: json!([block, index]),
            },
        )
        .await
    }

    /// Returns the number of transactions sent from an address.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_transaction_count(&mut self, block: BlockNumber) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionCount",
                params: json!([block]),
            },
        )
        .await
    }

    /// Returns information about a uncle of a block by number and uncle index position.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    /// * `index` - the uncle's index position.
    pub async fn get_uncle_by_block_number_and_index(
        &mut self,
        block: BlockNumber,
        index: U256,
    ) -> In3Result<Block> {
        rpc(
            self.client(),
            Request {
                method: "eth_getUncleByBlockNumberAndIndex",
                params: json!([block, index]),
            },
        )
        .await
    }

    /// Returns information about a uncle of a block by hash and uncle index position.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    /// * `index` - the uncle's index position.
    pub async fn get_uncle_by_block_hash_and_index(
        &mut self,
        hash: Hash,
        index: U256,
    ) -> In3Result<Block> {
        rpc(
            self.client(),
            Request {
                method: "eth_getUncleByBlockHashAndIndex",
                params: json!([hash, index]),
            },
        )
        .await
    }

    /// Returns the number of uncles in a block from a block matching the given block hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    pub async fn get_uncle_count_by_block_hash(&mut self, hash: Hash) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getUncleCountByBlockHash",
                params: json!([hash]),
            },
        )
        .await
    }

    /// Returns the number of uncles in a block from a block matching the given block number.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_uncle_count_by_block_number(&mut self, block: BlockNumber) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getUncleCountByBlockNumber",
                params: json!([block]),
            },
        )
        .await
    }

    /// Creates new message call transaction or a contract creation, if the data field contains
    /// code.
    ///
    /// # Arguments
    /// * `transaction` - [`OutgoingTransaction`](../types/struct.OutgoingTransaction.html)
    pub async fn send_transaction(&mut self, transaction: OutgoingTransaction) -> In3Result<Hash> {
        rpc(
            self.client(),
            Request {
                method: "eth_sendTransaction",
                params: json!([transaction]),
            },
        )
        .await
    }

    /// Creates new message call transaction or a contract creation for signed transactions.
    ///
    /// # Arguments
    /// * `data` - signed transaction data as bytes.
    pub async fn send_raw_transaction(&mut self, data: Bytes) -> In3Result<Hash> {
        rpc(
            self.client(),
            Request {
                method: "eth_sendRawTransaction",
                params: json!([data]),
            },
        )
        .await
    }

    /// Returns the receipt of a transaction by transaction hash.
    ///
    /// # Arguments
    /// * `transaction_hash` - hash of a transaction.
    pub async fn get_transaction_receipt(
        &mut self,
        transaction_hash: Hash,
    ) -> In3Result<TransactionReceipt> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionReceipt",
                params: json!([transaction_hash]),
            },
        )
        .await
    }
}

#[cfg(test)]
mod tests {
    const MOCK_DATA: &str = "../c/test/testdata/mock/{:?}.json";
    use std::convert::TryInto;

    use async_std::task;

    use async_trait::async_trait;

    use serde::Serialize;
    use crate::eth1::*;
    use crate::types::Bytes;
    use serde::Deserialize;
    use serde_json::json;

    use std::error::Error;
    use std::fmt::Write;
    use std::fs::File;
    use std::io::BufReader;
    use std::path::Path;

    use ethereum_types::{Address, U256};

    use crate::prelude::*;

    use super::*;

    fn init_api<'a>(transport: Box<dyn Transport>, chain: chain::ChainId, config: &'a str) -> Api {
        let mut client = Client::new(chain);
        // let _ = client.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
        let _ = client.configure(config);
        client.set_transport(transport);
        let mut api = Api::new(client);
        api
    }
    #[test]
    fn test_eth_api_block_number() -> In3Result<()> {
        //Make use of static string literals conversion for mock transport.
        let responses = vec![(
            "eth_blockNumber",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
        )];
        let transport: Box<dyn Transport> = Box::new(MockTransport {
            responses: responses,
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let num: u64 = task::block_on(eth_api.block_number())?.try_into().unwrap();
        println!("{:?}", num);
        assert_eq!(num, 0x96bacd);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_storage_at() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getStorageAt",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#).unwrap();
        let key: U256 = 0u64.into();
        let storage: u64 =
            task::block_on(eth_api.get_storage_at(address, key, BlockNumber::Earliest))?
                .try_into()
                .unwrap();
        println!("Storage value is {:?}", storage);
        assert_eq!(storage, 0x1);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_code() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getCode",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#)?;
        let code: Bytes = task::block_on(eth_api.get_code(address, BlockNumber::Latest))?
            .try_into()
            .unwrap();
        println!("Code at address {:?} is {:?}", address, code);
        assert!(code.0.len() > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_balance() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getBalance",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2""#)?;
        let balance: u64 =
            task::block_on(eth_api.get_balance(address, BlockNumber::Number((1555415).into())))?
                .try_into()
                .unwrap();
        println!("Balance of address {:?} is {:?} wei", address, balance);
        assert!(balance > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_block_by_number() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getBlockByNumber",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let block: Block = task::block_on(
            eth_api.get_block_by_number(BlockNumber::Number((1692767).into()), true),
        )?;
        println!("Block => {:?}", block,);

        let expected: U256 = (1692767).into();
        let blk: U256 = block.number.unwrap();
        println!("Block => {:?}", block);
        assert_eq!(blk, expected);
        Ok(())
    }

    #[test]
    fn test_eth_api_gas_price() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_gasPrice",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let gas_price: u64 = task::block_on(eth_api.gas_price())?.try_into().unwrap();
        println!("Gas price is {:?}", gas_price);
        assert!(gas_price > 1);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_block_by_hash() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getBlockByHash",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        // eth_getBlockByHash
        let hash: Hash = serde_json::from_str(
            r#""0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d""#,
        )?;
        let block: Block = task::block_on(eth_api.get_block_by_hash(hash, false))?;
        println!("Block => {:?}", block);
        let expected: U256 = (1550244).into();
        let blk: U256 = block.number.unwrap();
        println!("Block => {:?}", block);
        assert_eq!(blk, expected);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_logs() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_getLogs",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        // eth_getLogs
        let logs: Vec<Log> = task::block_on(eth_api.get_logs(serde_json::json!({
        "blockHash": "0x468f88ed8b40d940528552f093a11e4eb05991c787608139c931b0e9782ec5af",
        "topics": ["0xa61b5dec2abee862ab0841952bfbc161b99ad8c14738afa8ed8d5c522cd03946"]
        })))?;
        assert!(logs.len() > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_call() -> In3Result<()> {
        let responses = vec![(
            "eth_call",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x00000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000784bfa9eb182c3a02dbeb5285e3dba92d717e07a000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002168747470733a2f2f696e332e736c6f636b2e69742f6d61696e6e65742f6e642d3100000000000000000000000000000000000000000000000000000000000000"}]"#,
        )];
        // let transport:Box<dyn Transport> = Box::new(MockTransport {
        //     responses: responses,
        // });
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {
            responses: "eth_call",
        });
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}},"verification":"none"}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let contract: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#).unwrap();
        let mut abi = abi::In3EthAbi::new();
        let params = task::block_on(abi.encode("hasAccess():bool", serde_json::json!([]))).unwrap();
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
        let access: U256 = serde_json::from_value(output).unwrap();
        println!("{:?}", access);
        let expected: U256 = (1).into();
        assert_eq!(access, expected);
        Ok(())
    }

    // #[test]
    // fn test_eth_api_new_filter() -> In3Result<()> {
    //     let responses = vec![(
    //         "eth_blockNumber",
    //         r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
    //     )];
    //     add_response("eth_blockNumber", "[]", "\"0x84cf52\"", NULL, NULL);
    //     json_ctx_t* jopt = parse_json("{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}");
    //     size_t      fid  = eth_newFilter(in3, jopt);
    //     test_eth_api_ASSERT_GREATER_THAN(0, fid);
    // }

    //     #[test]
    //     fn test_eth_api_new_block_filter() -> In3Result<()> {
    //         in3_t* in3 = init_in3(mock_transport, 0x5);
    //   // we can add any mock json as we need trasnport but we are not calling any rpc endpoint
    //   //get filter id for new block
    //   size_t fid = eth_newBlockFilter(in3);
    //   test_eth_api_ASSERT_TRUE(fid > 0);

    //     }

    // //     #[test]
    // //     fn test_eth_api_filters() -> In3Result<()> {

    // //     }

    // //     #[test]
    // let responses = vec![("eth_call",
    //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x00000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000784bfa9eb182c3a02dbeb5285e3dba92d717e07a000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002168747470733a2f2f696e332e736c6f636b2e69742f6d61696e6e65742f6e642d3100000000000000000000000000000000000000000000000000000000000000"}]"#
    //     )];

    // "{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}"
    // let transport:Box<dyn Transport> = Box::new(MockTransport {
    //     responses: responses,
    // });
    // / r#"[{"fromBlock":"0x84cf51","address":"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455","topics":["0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f"]}]"#

    #[test]
    fn test_eth_api_get_filter_changes() -> In3Result<()> {
        let config = r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let responses = vec![
            ("eth_getLogs", r#"[{"jsonrpc":"2.0","id":1,"result":""}]"#),
            (
                "eth_blockNumber",
                r#"[{"jsonrpc":"2.0","id":1,"result":"0x84cf55"}]"#,
            ),
        ];
        let transport: Box<dyn Transport> = Box::new(MockTransport {
            responses: responses,
        });
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let jopts = serde_json::json!({
        "fromBlock": "0x84cf51",
        "address":"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455",
        "topics": ["0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f"]
        });
        let fid = task::block_on(eth_api.new_filter(jopts))?;
        let ret: FilterChanges = task::block_on(eth_api.get_filter_changes(fid))?;
        println!("{:?}", ret);
        assert!(true);
        Ok(())
    }

    // //     #[test]
    // //     fn test_eth_api_get_filter_logs() -> In3Result<()> {
    // //         test_eth_api_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, 1, NULL));

    // //   // Create filter options
    // //   char b[30];
    // //   sprintf(b, "{\"fromBlock\":\"0x1ca181\"}");
    // //   json_ctx_t* jopt = parse_json(b);

    // //   // Create new filter with options
    // //   size_t fid = eth_newFilter(in3, jopt);

    // //   // Get logs
    // //   eth_log_t *logs = NULL, *l = NULL;
    // //   test_eth_api_ASSERT_EQUAL(IN3_OK, eth_getFilterLogs(in3, fid, &logs));

    // //   while (logs) {
    // //     l    = logs;
    // //     logs = logs->next;
    // //     eth_log_free(l);
    // //   }
    // //   eth_uninstallFilter(in3, fid);
    // //   json_free(jopt);

    // //   // Test with non-existent filter id
    // //   test_eth_api_ASSERT_EQUAL(IN3_EINVAL, eth_getFilterLogs(in3, 1234, NULL));

    // //   // Test with all filters uninstalled
    // //   test_eth_api_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, fid, NULL));
    // //     }

    //     #[test]
    //     fn test_eth_api_chain_id() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_block_transaction_count_by_hash() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_block_transaction_count_by_number() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_estimate_gas() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_transaction_by_hash() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_transaction_by_block_hash_and_index() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_transaction_by_block_number_and_index() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_transaction_count() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_uncle_by_block_number_and_index() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_uncle_count_by_block_hash() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_get_uncle_count_by_block_number() -> In3Result<()> {}

    //     #[test]
    //     fn test_eth_api_send_transaction() -> In3Result<()> {
    //         let responses = vec![
    //         (
    //             "eth_estimateGas",
    //             r#"[{"jsonrpc":"2.0","id":1,"result":"0x96c0"}]"#,
    //         ),
    //         (
    //             "eth_sendRawTransaction",
    //             r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
    //         ),
    //         (
    //             "eth_gasPrice",
    //             r#"[{"jsonrpc":"2.0","id":1,"result":"0x9184e72a000"}]"#,
    //         ),
    //         (
    //             "eth_getTransactionCount",
    //             r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
    //         ),
    //     ];
    //     eth_api.client().configure(
    //         r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    //     );
    //     eth_api
    //         .client()
    //         .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    //     eth_api.client().set_transport(Box::new(MockTransport {
    //         responses: responses,
    //     }));
    //     let mut abi = abi::In3EthAbi::new();
    //     let params = task::block_on(abi.encode(
    //         "setData(uint256,string)",
    //         serde_json::json!([123, "testdata"]),
    //     ))
    //     .unwrap();
    //     println!("{:?}", params);
    //     let to: Address =
    //         serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
    //     let from: Address =
    //         serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap();
    //     let txn = OutgoingTransaction {
    //         to: to,
    //         from: from,
    //         data: Some(params),
    //         ..Default::default()
    //     };

    //     let hash: Hash = task::block_on(eth_api.send_transaction(txn)).unwrap();
    //     println!("Hash => {:?}", hash);

    //     }

    //     #[test]
    //     fn test_eth_api_send_raw_transaction() -> In3Result<()> {
    //         let responses = vec![
    //             (
    //                 "eth_estimateGas",
    //                 r#"[{"jsonrpc":"2.0","id":1,"result":"0x96c0"}]"#,
    //             ),
    //             (
    //                 "eth_sendRawTransaction",
    //                 r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
    //             ),
    //             (
    //                 "eth_gasPrice",
    //                 r#"[{"jsonrpc":"2.0","id":1,"result":"0x9184e72a000"}]"#,
    //             ),
    //             (
    //                 "eth_getTransactionCount",
    //                 r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
    //             ),
    //         ];
    //         eth_api.client().configure(
    //             r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    //         );
    //         eth_api
    //             .client()
    //             .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    //         eth_api.client().set_transport(Box::new(MockTransport {
    //             responses: responses,
    //         }));
    //         let mut abi = abi::In3EthAbi::new();
    //         let params = task::block_on(abi.encode(
    //             "setData(uint256,string)",
    //             serde_json::json!([123, "testdata"]),
    //         ))
    //         .unwrap();
    //         println!("{:?}", params);
    //         let to: Address =
    //             serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
    //         let from: Address =
    //             serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap();
    //         let txn = OutgoingTransaction {
    //             to: to,
    //             from: from,
    //             data: Some(params),
    //             ..Default::default()
    //         };

    //         let hash: Hash =     task::block_on(eth_api.send_transaction(txn)).unwrap();
    //         println!("Hash => {:?}", hash);

    //     }

    //     #[test]
    //     fn test_eth_api_get_transaction_receipt() -> In3Result<()> {}
}
