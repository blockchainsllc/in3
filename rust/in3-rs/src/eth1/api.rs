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
    /// * `address` - [`BlockNumber`](../types/enum.BlockNumber.html)
    /// * `block` - [`BlockNumber`](../types/enum.BlockNumber.html)
    pub async fn get_transaction_count(
        &mut self,
        address: Address,
        block: BlockNumber,
    ) -> In3Result<U256> {
        rpc(
            self.client(),
            Request {
                method: "eth_getTransactionCount",
                params: json!([address, block]),
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
    use std::convert::TryInto;

    use async_std::task;
    use ethereum_types::{Address, U256};
    use rustc_hex::FromHex;

    use crate::eth1::*;
    use crate::prelude::*;
    use crate::types::Bytes;

    use super::*;

    fn init_api<'a>(transport: Box<dyn Transport>, chain: chain::ChainId, config: &'a str) -> Api {
        let mut client = Client::new(chain);
        let _ = client.configure(config);
        client.set_transport(transport);
        let api = Api::new(client);
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
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let num: u64 = task::block_on(eth_api.block_number())?
            .try_into()
            .expect("cannot convert to u64");
        println!("{:?}", num);
        assert_eq!(num, 0x96bacd);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_storage_at() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#).unwrap(); // cannot fail
        let key: U256 = 0u64.into();
        let storage: u64 =
            task::block_on(eth_api.get_storage_at(address, key, BlockNumber::Earliest))?
                .try_into()
                .expect("cannot convert to u64");
        println!("Storage value is {:?}", storage);
        assert_eq!(storage, 0x1);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_code() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#)?;
        let code: Bytes = task::block_on(eth_api.get_code(address, BlockNumber::Latest))?
            .try_into()
            .expect("cannot convert to bytes");
        println!("Code at address {:?} is {:?}", address, code);
        assert!(code.0.len() > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_balance() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2""#)?;
        let balance: u64 =
            task::block_on(eth_api.get_balance(address, BlockNumber::Number((1555415).into())))?
                .try_into()
                .expect("cannot convert to u64");
        println!("Balance of address {:?} is {:?} wei", address, balance);
        assert!(balance > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_block_by_number() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let block: Block = task::block_on(
            eth_api.get_block_by_number(BlockNumber::Number((1692767).into()), true),
        )?;
        let expected: U256 = (1692767).into();
        let blk: U256 = block.number.expect("missing block number in block");
        assert_eq!(blk, expected);
        Ok(())
    }

    #[test]
    fn test_eth_api_gas_price() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let gas_price: u64 = task::block_on(eth_api.gas_price())?
            .try_into()
            .expect("cannot convert to u64");
        println!("Gas price is {:?}", gas_price);
        assert!(gas_price > 1);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_block_by_hash() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        // eth_getBlockByHash
        let hash: Hash = serde_json::from_str(
            r#""0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d""#,
        )?;
        let block: Block = task::block_on(eth_api.get_block_by_hash(hash, false))?;
        println!("Block => {:?}", block);
        let expected: U256 = (1550244).into();
        let blk: U256 = block.number.expect("missing block number in block");
        println!("Block => {:?}", block);
        assert_eq!(blk, expected);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_logs() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut client = Client::new(chain::MAINNET);
        let _ = client.configure(config);
        client.set_transport(transport);
        let mut eth_api = Api::new(client);
        let logs: Vec<Log> = task::block_on(eth_api.get_logs(serde_json::json!({
        "fromBlock":"0x1ca181",
        })))?;
        assert!(logs.len() > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_call() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}},"verification":"none"}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let contract: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#).unwrap(); // cannot fail
        let mut abi = abi::In3EthAbi::new();
        let params = task::block_on(abi.encode("hasAccess():bool", serde_json::json!([])))
            .expect("ABI encode failed");
        let txn = CallTransaction {
            to: Some(contract),
            data: Some(params),
            ..Default::default()
        };
        let output: Bytes = task::block_on(eth_api.call(txn, BlockNumber::Latest))?
            .try_into()
            .expect("cannot convert to bytes");
        let output = task::block_on(abi.decode("uint256", output)).expect("ABI decode failed");
        let access: U256 = serde_json::from_value(output).unwrap(); // cannot fail if decode succeeded
        println!("{:?}", access);
        let expected: U256 = (1).into();
        assert_eq!(access, expected);
        Ok(())
    }

    #[test]
    fn test_eth_api_chain_id() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        // let mut client = Client::new(chain::MAINNET);
        // let _ = client.configure(config);
        // let mut eth_api = Api::new(client);
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let ret: u64 = task::block_on(eth_api.chain_id())?
            .try_into()
            .expect("cannot convert to u64");
        assert_eq!(ret, 1);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_block_transaction_count_by_hash() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let hash: Hash = serde_json::from_str(
            r#""0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d""#,
        )?;
        let ret: u64 = task::block_on(eth_api.get_block_transaction_count_by_hash(hash))?
            .try_into()
            .expect("cannot convert to u64");
        assert_eq!(ret, 2u64);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_block_transaction_count_by_number() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let number = BlockNumber::Number((1692767).into());
        let ret: u64 = task::block_on(eth_api.get_block_transaction_count_by_number(number))?
            .try_into()
            .expect("cannot convert to u64");
        assert_eq!(ret, 6u64);
        Ok(())
    }

    #[test]
    fn test_eth_api_estimate_gas() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let contract: Address =
            serde_json::from_str(r#""0x36643F8D17FE745a69A2Fd22188921Fade60a98B""#).unwrap(); // cannot fail
        let mut abi = abi::In3EthAbi::new();
        let params = task::block_on(abi.encode("hasAccess():bool", serde_json::json!([])))
            .expect("ABI encode failed");
        let txn = CallTransaction {
            to: Some(contract),
            data: Some(params),
            ..Default::default()
        };
        let ret: u64 = task::block_on(eth_api.estimate_gas(txn, BlockNumber::Latest))?
            .try_into()
            .expect("cannot convert to u64");
        assert_eq!(ret, 22103u64);
        Ok(())
    }

    #[test]
    fn test_eth_api_get_transaction_by_hash() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let hash: Hash = serde_json::from_str(
            r#""0x9241334b0b568ef6cd44d80e37a0ce14de05557a3cfa98b5fd1d006204caf164""#,
        )?;
        let tx: Transaction = task::block_on(eth_api.get_transaction_by_hash(hash))?;
        let nonce = tx.nonce;
        let blk_number = tx
            .block_number
            .expect("missing block number in transaction");
        let gas = tx.gas;
        assert_eq!(nonce, (49).into());
        assert_eq!(gas, (41943).into());
        assert_eq!(blk_number, (1550244).into());
        Ok(())
    }

    #[test]
    fn test_eth_api_get_transaction_by_block_hash_and_index() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let hash: Hash = serde_json::from_str(
            r#""0xbaf52e8d5e9c7ece67b1c3a0788379a4f486d8ec50bbf531b3a6720ca03fe1c4""#,
        )?;
        let tx: Transaction =
            task::block_on(eth_api.get_transaction_by_block_hash_and_index(hash, (0).into()))?;
        let nonce = tx.nonce;
        let blk_number = tx
            .block_number
            .expect("missing block number in transaction");
        let gas = tx.gas;
        assert_eq!(nonce, (8).into());
        assert_eq!(gas, (21000).into());
        assert_eq!(blk_number, (1723267).into());
        Ok(())
    }

    #[test]
    fn test_eth_api_get_transaction_by_block_number_and_index() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let number = BlockNumber::Number((1723267).into());
        let tx: Transaction =
            task::block_on(eth_api.get_transaction_by_block_number_and_index(number, (0).into()))?
                .try_into()
                .unwrap();
        let nonce = tx.nonce;
        let blk_number = tx
            .block_number
            .expect("missing block number in transaction");
        let gas = tx.gas;
        assert_eq!(nonce, (8).into());
        assert_eq!(gas, (21000).into());
        assert_eq!(blk_number, (1723267).into());
        Ok(())
    }

    #[test]
    fn test_eth_api_get_transaction_count() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let address: Address =
            serde_json::from_str(r#""0x0de496ae79194d5f5b18eb66987b504a0feb32f2""#)?;
        let tx_count: u64 =
            task::block_on(eth_api.get_transaction_count(address, BlockNumber::Latest))?
                .try_into()
                .expect("cannot convert to u64");
        assert!(tx_count > 0);
        Ok(())
    }

    #[test]
    fn test_eth_api_send_raw_transaction() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        eth_api
            .client()
            .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
        let data = "f8da098609184e72a0008296c094f99dbd3cfc292b11f74deea9fa730825ee0b56f2849184e72ab87000ff86c088504a817c80082520894f99dbd3cfc292b11f74deea9fa730825ee0b56f288016345785d8a0000802da089a9217cedb1fbe05f815264a355d339693fb80e4dc508c36656d62fa18695eaa04a3185a9a31d7d1feabd3f8652a15628e498eea03e0a08fe736a0ad67735affff2ea0936324cf235541114275bb72b5acfb5a5c1f6f6e7f426c94806ff4093539bfaaa010a7482378b19ee0930a77c14b18c5664b3aa6c3ebc7420954d81263625d6d6a";
        let rawbytes: Bytes = FromHex::from_hex(data).unwrap().into(); // cannot fail
        let hash: Hash = task::block_on(eth_api.send_raw_transaction(rawbytes))
            .expect("ETH send raw transaction failed");
        println!("Hash => {:?}", hash);
        Ok(())
    }

    #[test]
    fn test_eth_api_send_transaction() -> In3Result<()> {
        // mock verified from etherscan tx: https://goerli.etherscan.io/tx/0xee051f86d1a55c58d8e828ac9e1fb60ecd7cd78de0e5e8b4061d5a4d6d51ae2a
        let responses = vec![(
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","result":"0xee051f86d1a55c58d8e828ac9e1fb60ecd7cd78de0e5e8b4061d5a4d6d51ae2a","id":2,"in3":{"lastValidatorChange":0,"lastNodeList":2837876,"execTime":213,"rpcTime":213,"rpcCount":1,"currentBlock":2850136,"version":"2.1.0"}}]"#,
        )];
        let transport: Box<dyn Transport> = Box::new(MockTransport {
            responses: responses,
        });
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x5":{"needsUpdate":false}}}}"#;
        let mut client = Client::new(chain::GOERLI);
        let _ = client.configure(config);
        client.set_pk_signer("dcb7b68bf23f6b29ffef8f316b0015bfd952385f26ae72befaf68cf0d0b6b1b6");
        client.set_transport(transport);
        let mut eth_api = Api::new(client);
        let to: Address =
            serde_json::from_str(r#""0x930e62afa9ceb9889c2177c858dc28810cedbf5d""#).unwrap(); // cannot fail
        let from: Address =
            serde_json::from_str(r#""0x25e10479a1AD17B895C45364a7D971e815F8867D""#).unwrap(); // cannot fail

        let data = "00";
        let rawbytes: Bytes = FromHex::from_hex(&data).unwrap().into(); // cannot fail
        let txn = OutgoingTransaction {
            to: to,
            from: from,
            gas: Some((0x668A0).into()),
            gas_price: Some((0xee6b28000i64).into()),
            value: Some((0x1bc16d674ec80000i64).into()),
            data: Some(rawbytes),
            nonce: Some(0x1i64.into()),
        };

        let hash: Hash =
            task::block_on(eth_api.send_transaction(txn)).expect("ETH send transaction failed");
        let expected_hash: Hash = serde_json::from_str(
            r#""0xee051f86d1a55c58d8e828ac9e1fb60ecd7cd78de0e5e8b4061d5a4d6d51ae2a""#,
        )
        .unwrap(); // cannot fail
        println!("Hash => {:?}", hash);
        assert_eq!(hash.to_string(), expected_hash.to_string());
        Ok(())
    }

    //FIX: internal blocknumber call issue #367
    #[test]
    #[ignore]
    fn test_eth_api_new_filter() -> In3Result<()> {
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let responses = vec![(
            "eth_newFilter",
            r#"{"jsonrpc":"2.0","result":"0x3","id":73}"#,
        )];
        let transport: Box<dyn Transport> = Box::new(MockTransport {
            responses: responses,
        });
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let jopts = serde_json::json!({
            "topics": ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef"],
            "blockHash":"0x40b6019185d6ee0112445fbe678438b6968bad2e6f24ae26c7bb75461428fd43"
        });
        let fid = task::block_on(eth_api.new_filter(jopts))?;
        let expected: U256 = (3).into();

        println!("{:?}", fid);
        assert_eq!(fid, expected);
        Ok(())
    }


    #[test]
    fn test_eth_api_get_filter_changes() -> In3Result<()> {
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let mut client = Client::new(chain::MAINNET);
        let _ = client.configure(config);
        client.set_transport(transport);
        let mut eth_api = Api::new(client);
        let jopts = serde_json::json!({
            "fromBlock":"0x1ca181"
        });
        let fid = task::block_on(eth_api.new_filter(jopts))?;
        let ret: FilterChanges = task::block_on(eth_api.get_filter_changes(fid))?;
        match ret {
            FilterChanges::Logs(vec) => assert!(vec.len() > 0),
            FilterChanges::BlockHashes(vec) => assert!(vec.len() > 0),
        }
        Ok(())
    }

    //FIX: Method not supported by nano
    #[test]
    #[ignore]
    fn test_eth_api_get_uncle_by_block_number_and_index() -> In3Result<()> {
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut client = Client::new(chain::MAINNET);
        let _ = client.configure(config);
        let mut eth_api = Api::new(client);
        let number = BlockNumber::Number((56160).into());
        let block: Block =
            task::block_on(eth_api.get_uncle_by_block_number_and_index(number, (0).into()))?;
        let blk = block.number.expect("missing block number in block");
        assert!(blk > (0).into());
        Ok(())
    }

    //FIX: Method not supported by nano
    #[test]
    #[ignore]
    fn test_eth_api_get_uncle_count_by_block_hash() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});

        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let hash: Hash = serde_json::from_str(
            r#""0x685b2226cbf6e1f890211010aa192bf16f0a0cba9534264a033b023d7367b845""#,
        )?;
        let count: u64 = task::block_on(eth_api.get_uncle_count_by_block_hash(hash))?
            .try_into()
            .expect("cannot convert to u64");
        assert!(count > 0u64);
        Ok(())
    }

    //FIX: Method not supported by nano
    #[test]
    #[ignore]
    fn test_eth_api_get_uncle_count_by_block_number() -> In3Result<()> {
        let transport: Box<dyn Transport> = Box::new(MockJsonTransport {});
        let config = r#"{"autoUpdateList":false,"requestCount":1,"maxAttempts":1,"nodes":{"0x1":{"needsUpdate":false}}}}"#;
        let mut eth_api = init_api(transport, chain::MAINNET, config);
        let number = BlockNumber::Number((56160).into());
        let count: U256 = task::block_on(eth_api.get_uncle_count_by_block_number(number))?
            .try_into()
            .unwrap();
        assert!(count > (0).into());
        Ok(())
    }
}
