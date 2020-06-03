//! Ethereum JSON RPC client API. This implementation is more or less consistent with the
//! [Ethereum JSON RPC wiki](https://github.com/ethereum/wiki/wiki/JSON-RPC).
use ethereum_types::{Address, U256};
use serde_json::json;

use crate::error::*;
use crate::eth1::{
    Block, BlockNumber, CallTransaction, FilterChanges, Hash, Log, OutgoingTransaction,
    Transaction, TransactionReceipt,
};
use crate::json_rpc::{Request, rpc};
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
        rpc(self.client(), Request {
            method: "eth_getStorageAt",
            params: json!([address, key, block]),
        }).await
    }

    /// Returns code at a given address.
    ///
    /// # Arguments
    /// * `address` - address
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_code(&mut self, address: Address, block: BlockNumber) -> In3Result<Bytes> {
        rpc(self.client(), Request {
            method: "eth_getCode",
            params: json!([address, block]),
        }).await
    }

    /// Returns the balance of the account of given address.
    ///
    /// # Arguments
    /// * `address` - address
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_balance(&mut self, address: Address, block: BlockNumber) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getBalance",
            params: json!([address, block]),
        }).await
    }

    /// Returns the number of most recent block.
    pub async fn block_number(&mut self) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_blockNumber",
            params: json!([]),
        }).await
    }

    /// Returns the current price per gas in wei.
    pub async fn gas_price(&mut self) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_gasPrice",
            params: json!([]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getBlockByNumber",
            params: json!([block, include_tx]),
        }).await
    }

    /// Returns information about a block by hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    /// * `include_tx` - boolean which if true returns the full transaction objects otherwise only
    /// the hashes of the transactions.
    pub async fn get_block_by_hash(&mut self, hash: Hash, include_tx: bool) -> In3Result<Block> {
        rpc(self.client(), Request {
            method: "eth_getBlockByHash",
            params: json!([hash, include_tx]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getLogs",
            params: json!([filter_options]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_newFilter",
            params: json!([filter_options]),
        }).await
    }

    /// Creates a filter in the node, to notify when a new block arrives.
    pub async fn new_block_filter(&mut self) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_newBlockFilter",
            params: json!([]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_uninstallFilter",
            params: json!([filter_id]),
        }).await
    }

    /// Polling method for a filter, which returns an array of logs which occurred since last poll.
    ///
    /// # Arguments
    /// * `filter_id` - id of filter that must be polled.
    pub async fn get_filter_changes(&mut self, filter_id: U256) -> In3Result<FilterChanges> {
        rpc(self.client(), Request {
            method: "eth_getFilterChanges",
            params: json!([filter_id]),
        }).await
    }

    /// Returns an array of all logs matching filter with given id.
    ///
    /// # Arguments
    /// * `filter_id` - id of filter that must be matched against.
    pub async fn get_filter_logs(&mut self, filter_id: U256) -> In3Result<Vec<Log>> {
        rpc(self.client(), Request {
            method: "eth_getFilterLogs",
            params: json!([filter_id]),
        }).await
    }

    /// Returns the currently configured chain id, a value used in replay-protected transaction
    /// signing as introduced by EIP-155.
    pub async fn chain_id(&mut self) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_chainId",
            params: json!([]),
        }).await
    }

    /// Returns the number of transactions in a block from a block matching the given block hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    pub async fn get_block_transaction_count_by_hash(&mut self, hash: Hash) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getBlockTransactionCountByHash",
            params: json!([hash]),
        }).await
    }

    /// Returns the number of transactions in a block matching the given block number.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_block_transaction_count_by_number(
        &mut self,
        block: BlockNumber,
    ) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getBlockTransactionCountByNumber",
            params: json!([block]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_call",
            params: json!([transaction, block]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_estimateGas",
            params: json!([transaction, block]),
        }).await
    }

    /// Returns the information about a transaction requested by transaction hash.
    ///
    /// # Arguments
    /// * `hash` - hash of transaction.
    pub async fn get_transaction_by_hash(&mut self, hash: Hash) -> In3Result<Transaction> {
        rpc(self.client(), Request {
            method: "eth_getTransactionByHash",
            params: json!([hash]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getTransactionByBlockHashAndIndex",
            params: json!([hash, index]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getTransactionByBlockNumberAndIndex",
            params: json!([block, index]),
        }).await
    }

    /// Returns the number of transactions sent from an address.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_transaction_count(&mut self, block: BlockNumber) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getTransactionCount",
            params: json!([block]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getUncleByBlockNumberAndIndex",
            params: json!([block, index]),
        }).await
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
        rpc(self.client(), Request {
            method: "eth_getUncleByBlockHashAndIndex",
            params: json!([hash, index]),
        }).await
    }

    /// Returns the number of uncles in a block from a block matching the given block hash.
    ///
    /// # Arguments
    /// * `hash` - hash of a block.
    pub async fn get_uncle_count_by_block_hash(&mut self, hash: Hash) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getUncleCountByBlockHash",
            params: json!([hash]),
        }).await
    }

    /// Returns the number of uncles in a block from a block matching the given block number.
    ///
    /// # Arguments
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_uncle_count_by_block_number(&mut self, block: BlockNumber) -> In3Result<U256> {
        rpc(self.client(), Request {
            method: "eth_getUncleCountByBlockNumber",
            params: json!([block]),
        }).await
    }

    /// Creates new message call transaction or a contract creation, if the data field contains
    /// code.
    ///
    /// # Arguments
    /// * `transaction` - [`OutgoingTransaction`](../types/struct.OutgoingTransaction.html)
    pub async fn send_transaction(&mut self, transaction: OutgoingTransaction) -> In3Result<Hash> {
        rpc(self.client(), Request {
            method: "eth_sendTransaction",
            params: json!([transaction]),
        }).await
    }

    /// Creates new message call transaction or a contract creation for signed transactions.
    ///
    /// # Arguments
    /// * `data` - signed transaction data as bytes.
    pub async fn send_raw_transaction(&mut self, data: Bytes) -> In3Result<Hash> {
        rpc(self.client(), Request {
            method: "eth_sendRawTransaction",
            params: json!([data]),
        }).await
    }

    /// Returns the receipt of a transaction by transaction hash.
    ///
    /// # Arguments
    /// * `transaction_hash` - hash of a transaction.
    pub async fn get_transaction_receipt(
        &mut self,
        transaction_hash: Hash,
    ) -> In3Result<TransactionReceipt> {
        rpc(self.client(), Request {
            method: "eth_getTransactionReceipt",
            params: json!([transaction_hash]),
        }).await
    }
}

#[cfg(test)]
mod tests {
    use std::convert::TryInto;

    use async_std::task;

    use async_trait::async_trait;

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
}
