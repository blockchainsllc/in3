# API Reference Python


Python bindings and library for in3. Go to our [readthedocs](https://in3.readthedocs.io/) page for more on usage.

This library is based on the [C version of Incubed](http://github.com/slockit/in3-c), which limits the compatibility for Cython, so please contribute by compiling it to your own platform and sending us a pull-request!


## Quickstart

### Install with pip 
 
```python
pip install in3
```

### In3 Client Standalone

```python
import in3

in3_client = in3.Client()
block_number = in3_client.eth.block_number()
print(block_number) # Mainnet's block number

in3_client.eth # ethereum module
in3_client.in3 # in3 module 
```

### Tests
```bash
python tests/test_suite.py
```

### Contributing
1. Read the index and respect the architecture. For additional packages and files please update the index.
2. (Optional) Get the latest `libin3.dylib` from the Gitlab Pipeline on the `in-core` project and replace it in `in3/bind` folder. 
3. Write the changes in a new branch, then make a pull request for the `develop` branch when all tests are passing. Be sure to add new tests to the CI. 

### Index
Explanation of this source code architecture and how it is organized. For more on design-patterns see [here](http://geekswithblogs.net/joycsharp/archive/2012/02/19/design-patterns-for-model.aspx) or on [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.

- **in3.__init__.py**: Library entry point, imports organization. Standard for any pipy package.
- **in3.eth**: Package for Ethereum objects and tools.
- **in3.eth.account**: Api for managing simple wallets and smart-contracts alike.
- **in3.eth.api**: Ethereum tools and Domain Objects.
- **in3.eth.model**: Value Objects for Ethereum. 
- **in3.libin3**: Package for everything related to binding libin3 to python. Libin3 is written in C and can be found [here](https://github.com/slockit/in3-c).
- **in3.libin3.shared**: Native shared libraries for multiple operating systems and platforms.
- **in3.libin3.enum**: Enumerations mapping C definitions to python.
- **in3.libin3.lib_loader**: Bindings using Ctypes.
- **in3.libin3.runtime**: Runtime object, bridging the remote procedure calls to the libin3 instances. 
## Examples

### block_number

source : [in3-c/python/examples/block_number.py](https://github.com/slockit/in3-c/blob/master/python/examples/block_number.py)



```python
import in3


print('\nEthereum Main Network')
client = in3.Client()
latest_block = client.eth.block_number()
gas_price = client.eth.gas_price()
print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))

print('\nEthereum Kovan Test Network')
client = in3.Client('kovan')
latest_block = client.eth.block_number()
gas_price = client.eth.gas_price()
print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))

print('\nEthereum Goerli Test Network')
client = in3.Client('goerli')
latest_block = client.eth.block_number()
gas_price = client.eth.gas_price()
print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))

# Results Example
"""
Ethereum Main Network
Latest BN: 9801135
Gas Price: 2000000000 Wei

Ethereum Kovan Test Network
Latest BN: 17713464
Gas Price: 6000000000 Wei

Ethereum Goerli Test Network
Latest BN: 2460853
Gas Price: 4610612736 Wei
"""
```

### in3_config

source : [in3-c/python/examples/in3_config.py](https://github.com/slockit/in3-c/blob/master/python/examples/in3_config.py)



```python
import in3

if __name__ == '__main__':

    print('\nEthereum Goerli Test Network')
    goerli_cfg = in3.ClientConfig(chain_id=str(in3.Chain.GOERLI), node_signatures=3, request_timeout=10000)
    client = in3.Client(goerli_cfg)
    node_list = client.node_list()
    print('\nIncubed Registry:')
    print('\ttotal servers:', node_list.totalServers)
    print('\tlast updated in block:', node_list.lastBlockNumber)
    print('\tregistry ID:', node_list.registryId)
    print('\tcontract address:', node_list.contract)
    print('\nNodes Registered:\n')
    for node in node_list.nodes:
        print('\turl:', node.url)
        print('\tdeposit:', node.deposit)
        print('\tweight:', node.weight)
        print('\tregistered in block:', node.registerTime)
        print('\n')

# Results Example
"""
Ethereum Goerli Test Network

Incubed Registry:
	total servers: 7
	last updated in block: 2320627
	registry ID: 0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea
	contract address: 0x5f51e413581dd76759e9eed51e63d14c8d1379c8

Nodes Registered:

	url: https://in3-v2.slock.it/goerli/nd-1
	deposit: 10000000000000000
	weight: 2000
	registered in block: 1576227711


	url: https://in3-v2.slock.it/goerli/nd-2
	deposit: 10000000000000000
	weight: 2000
	registered in block: 1576227741


	url: https://in3-v2.slock.it/goerli/nd-3
	deposit: 10000000000000000
	weight: 2000
	registered in block: 1576227801


	url: https://in3-v2.slock.it/goerli/nd-4
	deposit: 10000000000000000
	weight: 2000
	registered in block: 1576227831


	url: https://in3-v2.slock.it/goerli/nd-5
	deposit: 10000000000000000
	weight: 2000
	registered in block: 1576227876


	url: https://tincubeth.komputing.org/
	deposit: 10000000000000000
	weight: 1
	registered in block: 1578947320


	url: https://h5l45fkzz7oc3gmb.onion/
	deposit: 10000000000000000
	weight: 1
	registered in block: 1578954071
"""
```



### Running the examples

To run an example, you need to install in3 first:
```sh
pip install in3
```

Copy one of the examples, or both and paste into a file, i.e. `example.py`:

`MacOS`
```sh
pbpaste > example.py
```

Execute the example with python:
```
python example.py
```

## in3


### Client
```python
Client(self, in3_config: ClientConfig = None)
```

Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
Once with the latest list at hand, the client can request any other on-chain information using the same scheme.

**Arguments**:

- `in3_config` _ClientConfig or str_ - (optional) Configuration for the client. If not provided, default is loaded.
  

#### node_list
```python
Client.node_list()
```

Gets the list of Incubed nodes registered in the selected chain registry contract.

**Returns**:

- `node_list` _NodeList_ - List of registered in3 nodes and metadata.
  

#### abi_encode
```python
Client.abi_encode(fn_signature: str, *fn_args)
```

Smart-contract ABI encoder. Used to serialize a rpc to the EVM.
Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
Note: Parameters refers to the list of variables in a method declaration.
Arguments are the actual values that are passed in when the method is invoked.
When you invoke a method, the arguments used must match the declaration's parameters in type and order.

**Arguments**:

- `fn_signature` _str_ - Function name, with parameters. i.e. `getBalance(uint256):uint256`, can contain the return types but will be ignored.
- `fn_args` _tuple_ - Function parameters, in the same order as in passed on to method_name.

**Returns**:

- `encoded_fn_call` _str_ - i.e. "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890"
  

#### abi_decode
```python
Client.abi_decode(fn_return_types: str, encoded_values: str)
```

Smart-contract ABI decoder. Used to parse rpc responses from the EVM.
Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)

**Arguments**:

- `fn_return_types` - Function return types. e.g. `uint256`, `(address,string,uint256)` or `getBalance(address):uint256`.
  In case of the latter, the function signature will be ignored and only the return types will be parsed.
- `encoded_values` - Abi encoded values. Usually the string returned from a rpc to the EVM.

**Returns**:

- `decoded_return_values` _tuple_ - "0x1234567890123456789012345678901234567890", "0x05"
  

#### call
```python
Client.call(transaction: RawTransaction, block_number: int)
```

Calls a smart-contract method that does not store the computation. Will be executed locally by Incubed's EVM.
curl localhost:8545 -X POST --data '{"jsonrpc":"2.0", "method":"eth_call", "params":[{"from": "eth.accounts[0]", "to": "0x65da172d668fbaeb1f60e206204c2327400665fd", "data": "0x6ffa1caa0000000000000000000000000000000000000000000000000000000000000005"}, "latest"], "id":1}'
Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.

**Arguments**:

  transaction (RawTransaction):
- `block_number` _int or str_ - Desired block number integer or 'latest', 'earliest', 'pending'.

**Returns**:

- `method_returned_value` - A hexadecimal. For decoding use in3.abi_decode.
  

### ClientConfig
```python
ClientConfig(
self,
chain_id: str = 'mainnet',
chain_finality_threshold: int = 10,
account_private_key: str = None,
latest_block_stall: int = 10,
node_signatures: int = 3,
node_signature_consensus: int = 1,
node_min_deposit: int = 10000000000000000,
node_list_auto_update: bool = True,
node_limit: int = None,
request_timeout: int = 5000,
request_retries: int = 0,
response_proof_level: In3ProofLevel = <In3ProofLevel.STANDARD: 'standard'>,
response_includes_code: bool = False,
response_keep_proof: bool = False,
cached_blocks: int = 1,
cached_code_bytes: 100 = 101____)
```

In3 Client Configuration class.
Determines the behavior of client, which chain to connect to, verification policy, update cycle, minimum number of
signatures collected on every request, and response timeout.
Those are the settings that determine information security levels. Considering integrity is guaranteed by and
confidentiality is not available on public blockchains, these settings will provide a balance between availability,
and financial stake in case of repudiation. The "newer" the block is, or the closest to "latest", the higher are
the chances it gets repudiated (a fork) by the chain, making lower the chances a node will sign on such information
and thus reducing its availability. Up to a certain point, the older the block gets, the highest is its
availability because of the close-to-zero repudiation risk. Blocks older than circa one year are stored in Archive
Nodes, expensive computers, so, despite of the zero repudiation risk, there are not many nodes and they must search
for the requested block in its database, lowering the availability as well. The verification policy enforces an
extra step of security, that proves important in case you have only one response from an archive node
and want to run a local integrity check, just to be on the safe side.

**Arguments**:

- `chain_id` _str_ - (optional) - 'main'|'goerli'|'kovan' Chain-id based on EIP-155. If None provided, will connect to the Ethereum network. example: 0x1 for mainNet
- `chain_finality_threshold` _int_ - (optional) - Behavior depends on the chain consensus algorithm: POA - percent of signers needed in order reach finality (% of the validators) i.e.: 60 %. POW - mined blocks on top of the requested, i.e. 8 blocks. Defaults are defined in enum.Chain.
- `latest_block_stall` _int_ - (optional) - Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes. example: 10 - will ask for the state from (latestBlock-10).
- `account_private_key` _str_ - (optional) - Account SK to sign requests. example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
- `node_signatures` _int_ - (optional) - Node signatures attesting the response to your request. Will send a separate request for each. example: 3 nodes will have to sign the response.
- `node_signature_consensus` _int_ - - Useful when signatureCount <= 1. The client will check for consensus in responses. example: 10 - will ask for 10 different nodes and compare results looking for a consensus in the responses.
- `node_min_deposit` _int_ - - Only nodes owning at least this amount will be chosen to sign responses to your requests. i.e. 1000000000000000000 Wei
- `node_list_auto_update` _bool_ - (optional) - If true the nodelist will be automatically updated. False may compromise data security.
- `node_limit` _int_ - (optional) - Limit nodes stored in the client. example: 150 nodes
- `request_timeout` _int_ - Milliseconds before a request times out. example: 100000 ms
- `request_retries` _int_ - (optional) - Maximum times the client will retry to contact a certain node. example: 10 retries
- `response_proof_level` _str_ - (optional) - 'none'|'standard'|'full' Full gets the whole block Patricia-Merkle-Tree, Standard only verifies the specific tree branch concerning the request, None only verifies the root hashes, like a light-client does.
- `response_includes_code` _bool_ - (optional) - If true, every request with the address field will include the data, if existent, that is stored in that wallet/smart-contract. If false, only the code digest is included.
- `response_keep_proof` _bool_ - (optional) - If true, proof data will be kept in every rpc response. False will remove this data after using it to verify the responses. Useful for debugging and manually verifying the proofs.
- `cached_blocks` _int_ - (optional) - Maximum blocks kept in memory. example: 100 last requested blocks
- `cached_code_bytes` _int_ - (optional) - Maximum number of bytes used to cache EVM code in memory. example: 100000 bytes
  

### NodeList
```python
NodeList(self, nodes: [<class 'in3.model.In3Node'>], contract: Account,
registryId: str, lastBlockNumber: int, totalServers: int)
```

List of incubed nodes and its metadata, in3 registry contract from which the list was taken,
network/registry id, and last block number in the selected chain.

**Arguments**:

- `nodes` _[In3Node]_ - list of incubed nodes
- `contract` _Account_ - incubed registry contract from which the list was taken
- `registryId` _str_ - uuid of this incubed network. one chain could contain more than one incubed networks.
- `lastBlockNumber` _int_ - last block signed by the network
- `totalServers` _int_ - Total servers number (for integrity?)
  

### In3Node
```python
In3Node(self, url: str, address: Account, index: int, deposit: int,
props: int, timeout: int, registerTime: int, weight: int)
```

Registered remote verifier that attest, by signing the block hash, that the requested block and transaction were
indeed mined are in the correct chain fork.

**Arguments**:

- `url` _str_ - Endpoint to post to example: https://in3.slock.it
- `index` _int_ - Index within the contract example: 13
- `address` _in3.Account_ - Address of the node, which is the public address it iis signing with. example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
- `deposit` _int_ - Deposit of the node in wei example: 12350000
- `props` _int_ - Properties of the node. example: 3
- `timeout` _int_ - Time (in seconds) until an owner is able to receive his deposit back after he unregisters himself example: 3600
- `registerTime` _int_ - When the node was registered in (unixtime?)
- `weight` _int_ - Score based on qualitative metadata to base which nodes to ask signatures from.
  

## in3.eth


### EthereumApi
```python
EthereumApi(self, runtime: In3Runtime, chain_id: str)
```

Module based on Ethereum's api and web3.js


#### keccak256
```python
EthereumApi.keccak256(message: str)
```

Keccak-256 digest of the given data. Compatible with Ethereum but not with SHA3-256.

**Arguments**:

- `message` _str_ - Message to be hashed.

**Returns**:

- `digest` _str_ - The message digest.
  

#### gas_price
```python
EthereumApi.gas_price()
```

The current gas price in Wei (1 ETH equals 1000000000000000000 Wei ).

**Returns**:

- `price` _int_ - minimum gas value for the transaction to be mined
  

#### block_number
```python
EthereumApi.block_number()
```

Returns the number of the most recent block the in3 network can collect signatures to verify.
Can be changed by Client.Config.replaceLatestBlock.
If you need the very latest block, change Client.Config.signatureCount to zero.

**Returns**:

  block_number (int) : Number of the most recent block
  

#### get_balance
```python
EthereumApi.get_balance(address: str, at_block: int = 'latest')
```

Returns the balance of the account of given address.

**Arguments**:

- `address` _str_ - address to check for balance
- `at_block` _int or str_ - block number IN3BlockNumber  or EnumBlockStatus

**Returns**:

- `balance` _int_ - integer of the current balance in wei.
  

#### get_storage_at
```python
EthereumApi.get_storage_at(
address: str,
position: int = 0,
at_block: int = <BlockStatus.LATEST: 'latest'>)
```

Stored value in designed position at a given address. Storage can be used to store a smart contract state, constructor or just any data.
Each contract consists of a EVM bytecode handling the execution and a storage to save the state of the contract.
The storage is essentially a key/value store. Use get_code to get the smart-contract code.

**Arguments**:

- `address` _str_ - Ethereum account address
- `position` _int_ - Position index, 0x0 up to 0x64
- `at_block` _int or str_ - Block number

**Returns**:

- `storage_at` _str_ - Stored value in designed position. Use decode('hex') to see ascii format of the hex data.
  

#### get_code
```python
EthereumApi.get_code(address: str,
at_block: int = <BlockStatus.LATEST: 'latest'>)
```

Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.

**Arguments**:

- `address` _str_ - Ethereum account address
- `at_block` _int or str_ - Block number

**Returns**:

- `bytecode` _str_ - Smart-Contract bytecode in hexadecimal.
  

#### get_transaction_count
```python
EthereumApi.get_transaction_count(
address: str, at_block: int = <BlockStatus.LATEST: 'latest'>)
```

Number of transactions mined from this address. Used to set transaction nonce.
Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
It exists to mitigate replay attacks.

**Arguments**:

- `address` _str_ - Ethereum account address
- `at_block` _int_ - Block number

**Returns**:

- `tx_count` _int_ - Number of transactions mined from this address.
  

#### get_block_by_hash
```python
EthereumApi.get_block_by_hash(block_hash: str,
get_full_block: bool = False)
```

Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined (get_block_by_number).

**Arguments**:

- `block_hash` _str_ - Desired block hash
- `get_full_block` _bool_ - If true, returns the full transaction objects, otherwise only its hashes.

**Returns**:

- `block` _Block_ - Desired block, if exists.
  

#### get_block_by_number
```python
EthereumApi.get_block_by_number(block_number: [<class 'int'>],
get_full_block: bool = False)
```

Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree (this) (get_block_by_hash).

**Arguments**:

- `block_number` _int or str_ - Desired block number integer or 'latest', 'earliest', 'pending'.
- `get_full_block` _bool_ - If true, returns the full transaction objects, otherwise only its hashes.

**Returns**:

- `block` _Block_ - Desired block, if exists.
  

#### get_transaction_by_hash
```python
EthereumApi.get_transaction_by_hash(tx_hash: str)
```

Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.

**Arguments**:

- `tx_hash` - Transaction hash.

**Returns**:

- `transaction` - Desired transaction, if exists.
  

### EthAccountApi
```python
EthAccountApi(self, runtime: In3Runtime, factory: EthObjectFactory)
```

Manages wallets and smart-contracts


#### sign
```python
EthAccountApi.sign(address: str, data: str)
```

Use ECDSA to sign a message.

**Arguments**:

- `address` _str_ - Ethereum address of the wallet that will sign the message.
- `data` _str_ - Data to be signed, EITHER a hash string or a Transaction.

**Returns**:

- `signed_message` _str_ - ECDSA calculated r, s, and parity v, concatenated. v = 27 + (r % 2)
  

#### send_transaction
```python
EthAccountApi.send_transaction(transaction: RawTransaction)
```

Signs and sends the assigned transaction. Requires the 'key' value to be set in ClientConfig.
Transactions change the state of an account, just the balance, or additionally, the storage and the code.
Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the
gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the
latest block.

**Arguments**:

- `transaction` - All information needed to perform a transaction. Minimum is from, to and value.
  Client will add the other required fields, gas and chaindId.

**Returns**:

- `tx_hash` - Transaction hash, used to get the receipt and check if the transaction was mined.
  

#### get_transaction_receipt
```python
EthAccountApi.get_transaction_receipt(tx_hash: str)
```

After a transaction is received the by the client, it returns the transaction hash. With it, it is possible to
gather the receipt, once a miner has mined and it is part of an acknowledged block. Because how it is possible,
in distributed systems, that data is asymmetric in different parts of the system, the transaction is only "final"
once a certain number of blocks was mined after it, and still it can be possible that the transaction is discarded
after some time. But, in general terms, it is accepted that after 6 to 8 blocks from latest, that it is very
likely that the transaction will stay in the chain.

**Arguments**:

- `tx_hash` - Transaction hash.

**Returns**:

  tx_receipt:
  

#### estimate_gas
```python
EthAccountApi.estimate_gas(transaction: RawTransaction)
```

Gas estimation for transaction. Used to fill transaction.gas field. Check RawTransaction docs for more on gas.

**Arguments**:

- `transaction` - Unsent transaction to be estimated. Important that the fields data or/and value are filled in.

**Returns**:

- `gas` _int_ - Calculated gas in Wei.
  

#### checksum_address
```python
EthAccountApi.checksum_address(address: str, add_chain_id: bool = True)
```

Will convert an upper or lowercase Ethereum address to a checksum address, that uses case to encode values.
See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md).

**Arguments**:

- `address` - Ethereum address string or object.
- `add_chain_id` _bool_ - Will append the chain id of the address, for multi-chain support, canonical for Eth.

**Returns**:

- `checksum_address` - EIP-55 compliant, mixed-case address object.
  

### in3.eth.model


#### DataTransferObject
```python
DataTransferObject()
```

Maps marshalling objects transferred to, and from a remote facade, in this case, libin3 rpc api.
For more on design-patterns see [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.


#### Transaction
```python
Transaction(self, From: str, to: str, gas: int, gasPrice: int, hash: str,
data: str, nonce: int, gasLimit: int, blockNumber: int,
transactionIndex: int, blockHash: str, value: int,
signature: str)
```

**Arguments**:

- `From` _hex str_ - Address of the sender account.
- `to` _hex str_ - Address of the receiver account. Left undefined for a contract creation transaction.
- `gas` _int_ - Gas for the transaction miners and execution in wei. Will get multiplied by `gasPrice`. Use in3.eth.account.estimate_gas to get a calculated value. Set too low and the transaction will run out of gas.
- `value` _int_ - Value transferred in wei. The endowment for a contract creation transaction.
- `data` _hex str_ - Either a ABI byte string containing the data of the function call on a contract, or in the case of a contract-creation transaction the initialisation code.
- `gasPrice` _int_ - Price of gas in wei, defaults to in3.eth.gasPrice. Also know as `tx fee price`. Set your gas price too low and your transaction may get stuck. Set too high on your own loss.
  gasLimit (int); Maximum gas paid for this transaction. Set by the client using this rationale if left empty: gasLimit = G(transaction) + G(txdatanonzero) × dataByteLength. Minimum is 21000.
- `nonce` _int_ - Number of transactions mined from this address. Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1). It exists to mitigate replay attacks. This allows to overwrite your own pending transactions by sending a new one with the same nonce. Use in3.eth.account.get_transaction_count to get the latest value.
- `hash` _hex str_ - Keccak of the transaction bytes, not part of the transaction. Also known as receipt, because this field is filled after the transaction is sent, by eth_sendTransaction
- `blockHash` _hex str_ - Block hash that this transaction was mined in. null when its pending.
- `blockNumber` _int_ - Block number that this transaction was mined in. null when its pending.
- `transactionIndex` _int_ - Integer of the transactions index position in the block. null when its pending.
- `signature` _hex str_ - ECDSA of transaction.data, calculated r, s and v concatenated. V is parity set by v = 27 + (r % 2).
  

#### RawTransaction
```python
RawTransaction(self,
From: str,
to: str,
gas: int,
nonce: int,
value: int = None,
data: str = None,
gasPrice: int = None,
gasLimit: int = None,
hash: str = None,
signature: str = None)
```

Unsent transaction. Use to send a new transaction.

**Arguments**:

- `From` _hex str_ - Address of the sender account.
- `to` _hex str_ - Address of the receiver account. Left undefined for a contract creation transaction.
- `gas` _int_ - Gas for the transaction miners and execution in wei. Will get multiplied by `gasPrice`. Use in3.eth.account.estimate_gas to get a calculated value. Set too low and the transaction will run out of gas.
- `value` _int_ - (optional) Value transferred in wei. The endowment for a contract creation transaction.
- `data` _hex str_ - (optional) Either a ABI byte string containing the data of the function call on a contract, or in the case of a contract-creation transaction the initialisation code.
- `gasPrice` _int_ - (optional) Price of gas in wei, defaults to in3.eth.gasPrice. Also know as `tx fee price`. Set your gas price too low and your transaction may get stuck. Set too high on your own loss.
  gasLimit (int); (optional) Maximum gas paid for this transaction. Set by the client using this rationale if left empty: gasLimit = G(transaction) + G(txdatanonzero) × dataByteLength. Minimum is 21000.
- `nonce` _int_ - (optional) Number of transactions mined from this address. Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1). It exists to mitigate replay attacks. This allows to overwrite your own pending transactions by sending a new one with the same nonce. Use in3.eth.account.get_transaction_count to get the latest value.
- `hash` _hex str_ - (optional) Keccak of the transaction bytes, not part of the transaction. Also known as receipt, because this field is filled after the transaction is sent.
- `signature` _hex str_ - (optional) ECDSA of transaction, r, s and v concatenated. V is parity set by v = 27 + (r % 2).
  

#### Filter
```python
Filter(self, fromBlock: int, toBlock: int, address: str, topics: list,
blockhash: str)
```

Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
transaction to the designed address, and has a word listed on topics.


#### Account
```python
Account(self, address: str, chain_id: str, secret: str = None)
```

Ethereum address of a wallet or smart-contract


## in3.libin3


### In3Runtime
```python
In3Runtime(self, timeout: int)
```

Instantiate libin3 and frees it when garbage collected.

**Arguments**:

- `timeout` _int_ - Time for http request connection and content timeout in milliseconds
  

#### call
```python
In3Runtime.call(fn_name: str, *args)
```

Make a remote procedure call to a function in libin3

**Arguments**:

- `fn_name` _str or Enum_ - Name of the function to be called
- `*args` - Arguments matching the parameters order of this function

**Returns**:

- `fn_return` _str_ - String of values returned by the function, if any.
  

### in3.libin3.lib_loader

Load libin3 shared library for the current system, map function signatures, map and set transport functions.

Example of RPC to In3-Core library, In3 Network and back.
```
+----------------+                               +----------+                       +------------+                        +------------------+
|                | in3.client.eth.block_number() |          |  in3_client_exec_req  |            |  In3 Network Request   |                  |e
|     python     +------------------------------>+  python  +----------------------->   libin3   +------------------------>     python       |
|   application  |                               |   in3    |                       |  in3-core  |                        |  http_transport  |
|                <-------------------------------+          <-----------------------+            <------------------------+                  |
+----------------+     primitive or Object       +----------+     ctype object      +------------+  in3_req_add_response  +------------------+
```


#### In3Request
```python
In3Request()
```

Request sent by the libin3 to the In3 Network, transported over the _http_transport function
Based on in3/client/.h in3_request_t struct

**Attributes**:

- `payload` _str_ - the payload to send
- `urls` _[str]_ - array of urls
- `urls_len` _int_ - number of urls
- `results` _str_ - the responses
- `timeout` _int_ - the timeout 0= no timeout
- `times` _int_ - measured times (in ms) which will be used for ajusting the weights
  

#### libin3_new
```python
libin3_new(timeout: int)
```

RPC to free libin3 objects in memory.

**Arguments**:

- `timeout` _int_ - Time in milliseconds for http requests to fail due to timeout

**Returns**:

- `instance` _int_ - Memory address of the shared library instance, return value from libin3_new
  

#### libin3_free
```python
libin3_free(instance: int)
```

RPC to free libin3 objects in memory.

**Arguments**:

- `instance` _int_ - Memory address of the shared library instance, return value from libin3_new
  

#### libin3_call
```python
libin3_call(instance: int, rpc: bytes)
```

Make Remote Procedure Call to an arbitrary method of a libin3 instance

**Arguments**:

- `instance` _int_ - Memory address of the shared library instance, return value from libin3_new
- `rpc` _bytes_ - Serialized function call, a ethreum api json string.

**Returns**:

- `returned_value` _object_ - The returned function value(s)
  
