# API Reference Python

## Python Incubed client

This library is based on the [C version of Incubed](http://github.com/slockit/in3-c), which limits the compatibility for Cython, so please contribute by compiling it to your own platform and sending us a pull-request!

Go to our [readthedocs](https://in3.readthedocs.io/) page for more.

### Quickstart

#### Install with pip 
 
```shell script
pip install in3
```

#### In3 Client API

```python
import in3

in3_client = in3.Client()
# Sends a request to the Incubed Network, that in turn will collect proofs from the Ethereum client, 
# attest and sign the response, then send back to the client, that will verify signatures and proofs. 
block_number = in3_client.eth.block_number()
print(block_number) # Mainnet's block number

in3_client  # incubed network api 
in3_client.eth  # ethereum api
in3_client.account  # ethereum account api
in3_client.contract  # ethereum smart-contract api
```

#### Developing & Tests
Install dev dependencies, IDEs should automatically recognize interpreter if done like this.
```
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```
Compile local libraries and run tests. Make sure you have cmake installed.
```bash
./buidl_libs.sh
```

#### Index
Explanation of this source code architecture and how it is organized. For more on design-patterns see [here](http://geekswithblogs.net/joycsharp/archive/2012/02/19/design-patterns-for-model.aspx) or on [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.

- **in3.__init__.py**: Library entry point, imports organization. Standard for any pipy package.
- **in3.client**: Incubed Client and API.
- **in3.model**: MVC Model classes for the Incubed client module domain.
- **in3.transport**: HTTP Transport function and error handling.
- **in3.wallet**: WiP - Wallet API.
- **in3.exception**: Custom exceptions. 
- **in3.eth**: Ethereum module.
- **in3.eth.api**: Ethereum API.
- **in3.eth.account**: Ethereum accounts.
- **in3.eth.contract**: Ethereum smart-contracts API.
- **in3.eth.model**: MVC Model classes for the Ethereum client module domain. Manages serializaation.
- **in3.eth.factory**: Ethereum Object Factory. Manages deserialization.
- **in3.libin3**: Module for the libin3 runtime. Libin3 is written in C and can be found [here](https://github.com/slockit/in3-c).
- **in3.libin3.shared**: Native shared libraries for multiple operating systems and platforms.
- **in3.libin3.enum**: Enumerations mapping C definitions to python.
- **in3.libin3.lib_loader**: Bindings using Ctypes.
- **in3.libin3.runtime**: Runtime object, bridging the remote procedure calls to the libin3 instances. 

## Examples

### connect_to_ethereum

source : [in3-c/python/examples/connect_to_ethereum.py](https://github.com/slockit/in3-c/blob/master/python/examples/connect_to_ethereum.py)



```python
"""
Connects to Ethereum and fetches attested information from each chain.
"""
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

# Produces
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

### incubed_network

source : [in3-c/python/examples/incubed_network.py](https://github.com/slockit/in3-c/blob/master/python/examples/incubed_network.py)



```python
"""
Shows Incubed Network Nodes Stats
"""
import in3

print('\nEthereum Goerli Test Network')
client = in3.Client('goerli')
node_list = client.refresh_node_list()
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

# Produces
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

### resolve_eth_names

source : [in3-c/python/examples/resolve_eth_names.py](https://github.com/slockit/in3-c/blob/master/python/examples/resolve_eth_names.py)



```python
"""
Resolves ENS domains to Ethereum addresses
ENS is a smart-contract system that registers and resolves `.eth` domains.
"""
import in3


def _print():
    print('\nAddress for {} @ {}: {}'.format(domain, chain, address))
    print('Owner for {} @ {}: {}'.format(domain, chain, owner))


# Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
domain = 'depraz.eth'

print('\nEthereum Name Service')

# Instantiate In3 Client for Goerli
chain = 'goerli'
client = in3.Client(chain, cache_enabled=False)
address = client.ens_address(domain)
# owner = client.ens_owner(domain)
# _print()

# Instantiate In3 Client for Mainnet
chain = 'mainnet'
client = in3.Client(chain, cache_enabled=False)
address = client.ens_address(domain)
owner = client.ens_owner(domain)
_print()

# Instantiate In3 Client for Kovan
chain = 'kovan'
client = in3.Client(chain, cache_enabled=True)
try:
    address = client.ens_address(domain)
    owner = client.ens_owner(domain)
    _print()
except in3.ClientException:
    print('\nENS is not available on Kovan.')


# Produces
"""
Ethereum Name Service

Address for depraz.eth @ mainnet: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for depraz.eth @ mainnet: 0x6fa33809667a99a805b610c49ee2042863b1bb83

ENS is not available on Kovan.
"""

```

### send_transaction

source : [in3-c/python/examples/send_transaction.py](https://github.com/slockit/in3-c/blob/master/python/examples/send_transaction.py)



```python
"""
Sends Ethereum transactions using Incubed.
Incubed send Transaction does all necessary automation to make sending a transaction a breeze.
Works with included `data` field for smart-contract calls.
"""
import json
import in3
import time


# On Metamask, be sure to be connected to the correct chain, click on the `...` icon on the right corner of
# your Account name, select `Account Details`. There, click `Export Private Key`, copy the value to use as secret.
# By reading the terminal input, this value will stay in memory only. Don't forget to cls or clear terminal after ;)
sender_secret = input("Sender secret: ")
receiver = input("Receiver address: ")
#     1000000000000000000 == 1 ETH
#              1000000000 == 1 Gwei Check https://etherscan.io/gasTracker.
value_in_wei = 1463926659
# None for Eth mainnet
chain = 'goerli'
client = in3.Client(chain if chain else 'mainnet')
# A transaction is only final if a certain number of blocks are mined on top of it.
# This number varies with the chain's consensus algorithm. Time can be calculated over using:
# wait_time = blocks_for_consensus * avg_block_time_in_secs
# For mainnet and paying low gas, it might take 10 minutes.
confirmation_wait_time_in_seconds = 30
etherscan_link_mask = 'https://{}{}etherscan.io/tx/{}'

print('-= Ethereum Transaction using Incubed =- \n')
try:
    sender = client.eth.account.recover(sender_secret)
    tx = in3.eth.NewTransaction(to=receiver, value=value_in_wei)
    print('[.] Sending {} Wei from {} to {}. Please wait.\n'.format(tx.value, sender.address, tx.to))
    tx_hash = client.eth.account.send_transaction(sender, tx)
    print('[.] Transaction accepted with hash {}.'.format(tx_hash))
    add_dot_if_chain = '.' if chain else ''
    print(etherscan_link_mask.format(chain, add_dot_if_chain, tx_hash))
    while True:
        try:
            print('\n[.] Waiting {} seconds for confirmation.\n'.format(confirmation_wait_time_in_seconds))
            time.sleep(confirmation_wait_time_in_seconds)
            receipt: in3.eth.TransactionReceipt = client.eth.transaction_receipt(tx_hash)
            print('[.] Transaction was sent successfully!\n')
            print(json.dumps(receipt.to_dict(), indent=4, sort_keys=True))
            print('[.] Mined on block {} used {} GWei.'.format(receipt.blockNumber, receipt.gasUsed))
            break
        except Exception:
            print('[!] Transaction not mined yet, check https://etherscan.io/gasTracker.')
            print('[!] Just wait some minutes longer than the average for the price paid!')
except in3.PrivateKeyNotFoundException as e:
    print(str(e))
except in3.ClientException as e:
    print('Client returned error: ', str(e))
    print('Please try again.')

# Response
"""
Ethereum Transaction using Incubed

Sending 1463926659 Wei from 0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308 to 0x6fa33809667a99a805b610c49ee2042863b1bb83.

Transaction accepted with hash 0xbeebda39e31e42d2a26476830fdcdc2d21e9df090af203e7601d76a43074d8d3.
https://goerli.etherscan.io/tx/0xbeebda39e31e42d2a26476830fdcdc2d21e9df090af203e7601d76a43074d8d3

Waiting 25 seconds for confirmation.

Transaction was sent successfully!
{
    "From": "0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308",
    "blockHash": "0x9693714c9d7dbd31f36c04fbd262532e68301701b1da1a4ee8fc04e0386d868b",
    "blockNumber": 2615346,
    "cumulativeGasUsed": 21000,
    "gasUsed": 21000,
    "logsBloom": "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "status": 1,
    "to": "0x6FA33809667A99A805b610C49EE2042863b1bb83",
    "transactionHash": "0xbeebda39e31e42d2a26476830fdcdc2d21e9df090af203e7601d76a43074d8d3",
    "transactionIndex": 0
}

Mined on block 2615346 used 21000 GWei.
"""

```

### smart_contract

source : [in3-c/python/examples/smart_contract.py](https://github.com/slockit/in3-c/blob/master/python/examples/smart_contract.py)



```python
"""
Manually calling ENS smart-contract
![UML Sequence Diagram of how Ethereum Name Service ENS resolves a name.](https://lh5.googleusercontent.com/_OPPzaxTxKggx9HuxloeWtK8ggEfIIBKRCEA6BKMwZdzAfUpIY6cz7NK5CFmiuw7TwknbhFNVRCJsswHLqkxUEJ5KdRzpeNbyg8_H9d2RZdG28kgipT64JyPZUP--bAizozaDcxCq34)
"""
import in3


client = in3.Client('goerli')
domain_name = client.ens_namehash('depraz.eth')
ens_registry_addr = '0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e'
ens_resolver_abi = 'resolver(bytes32):address'

# Find resolver contract for ens name
resolver_tx = {
    "to": ens_registry_addr,
    "data": client.eth.contract.encode(ens_resolver_abi, domain_name)
}
tx = in3.eth.NewTransaction(**resolver_tx)
encoded_resolver_addr = client.eth.contract.call(tx)
resolver_address = client.eth.contract.decode(ens_resolver_abi, encoded_resolver_addr)

# Resolve name
ens_addr_abi = 'addr(bytes32):address'
name_tx = {
    "to": resolver_address,
    "data": client.eth.contract.encode(ens_addr_abi, domain_name)
}
encoded_domain_address = client.eth.contract.call(in3.eth.NewTransaction(**name_tx))
domain_address = client.eth.contract.decode(ens_addr_abi, encoded_domain_address)

print('END domain:\n{}\nResolved by:\n{}\nTo address:\n{}'.format(domain_name, resolver_address, domain_address))

# Produces
"""
END domain:
0x4a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e
Resolved by:
0x4b1488b7a6b320d2d721406204abc3eeaa9ad329
To address:
0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
"""

```

### smart_meter_write

source : [in3-c/python/examples/smart_meter_write.py](https://github.com/slockit/in3-c/blob/master/python/examples/smart_meter_write.py)



```python
"""
[{"type":"event","name":"NewReadStored","inputs":[{"type":"address","name":"meter","internalType":"address","indexed":true},{"type":"uint256","name":"bucket","internalType":"uint256","indexed":true},{"type":"address","name":"operator","internalType":"address","indexed":false},{"type":"uint256","name":"timestamp","internalType":"uint256","indexed":false},{"type":"bytes","name":"data","internalType":"bytes","indexed":false}],"anonymous":false},{"type":"function","stateMutability":"view","payable":false,"outputs":[{"type":"uint256","name":"","internalType":"uint256"}],"name":"DAILY_BUCKET","inputs":[],"constant":true},{"type":"function","stateMutability":"nonpayable","payable":false,"outputs":[],"name":"store","inputs":[{"type":"address","name":"meter","internalType":"address"},{"type":"bytes","name":"data","internalType":"bytes"},{"type":"uint256","name":"timestamp","internalType":"uint256"},{"type":"uint256","name":"bucket","internalType":"uint256"}],"constant":false},{"type":"function","stateMutability":"nonpayable","payable":false,"outputs":[],"name":"storeWithDailyBucket","inputs":[{"type":"address","name":"meter","internalType":"address"},{"type":"bytes","name":"data","internalType":"bytes"},{"type":"uint256","name":"timestamp","internalType":"uint256"}],"constant":false}]

0xf23FF7472FC62C6bEe2F960f5b4170Ab3C1C26d2
"""
import base64
import json

import in3
import hashlib
import random
import time

if __name__ == '__main__':

    c = in3.Client(chain='ewc', in3_config=in3.ClientConfig(transport_binary_format=True))

    smart_meter_registry_addr = '0xf23FF7472FC62C6bEe2F960f5b4170Ab3C1C26d2'
    # meter, bucket, operator, timestamp, data
    NewReadStoredEvent = 'NewReadStored(address,uint,address,uint,bytes))'
    try:
        # meter, data, timestamp
        storeWithDailyBucket = 'storeWithDailyBucket(address,bytes,uint256)'
        meter_addr = '0xb11469A59dF65312737053c4785FA7Ff28660013'
        salt = hex(random.getrandbits(64))
        secret_read = hashlib.sha512(b'1101101')
        secret_read.update(salt.encode('utf8'))
        secret_read = secret_read.hexdigest()
        # secret_read = base64.b64encode(secret_read.digest())
        timestamp = int(time.time())
        sender_secret = input("Sender secret: ")
        sender = c.eth.account.recover(sender_secret)
        encoded_contract_call = c.eth.contract.encode(storeWithDailyBucket, meter_addr, secret_read, timestamp)
        tx = in3.eth.NewTransaction(to=smart_meter_registry_addr,
                                    data=encoded_contract_call)
        tx.gasLimit = c.eth.account.estimate_gas(tx)
        tx_hash = c.eth.account.send_transaction(sender=sender, transaction=tx)
        print('https://explorer.energyweb.org/tx/{}'.format(tx_hash))
        confirmation_wait_time_in_seconds = 60
        while True:
            try:
                print('\n[.] Waiting {} seconds for confirmation.\n'.format(confirmation_wait_time_in_seconds))
                time.sleep(confirmation_wait_time_in_seconds)
                receipt: in3.eth.TransactionReceipt = c.eth.transaction_receipt(tx_hash)
                print('[.] Transaction was sent successfully!\n')
                print(json.dumps(receipt.to_dict(), indent=4, sort_keys=True))
                print('[.] Mined on block {} used {} GWei.'.format(receipt.blockNumber, receipt.gasUsed))
                break
            except Exception:
                print('[!] Transaction not mined yet, check https://etherscan.io/gasTracker.')
                print('[!] Just wait some minutes longer than the average for the price paid!')
    except in3.PrivateKeyNotFoundException as e:
        print(str(e))
    except in3.ClientException as e:
        print('Client returned error: ', str(e))
        print('Please try again.')

```


### Running the examples

To run an example, you need to install in3 first:
```sh
pip install in3
```
This will install the library system-wide. Please consider using `virtualenv` or `pipenv` for a project-wide install.

Then copy one of the examples and paste into a file, i.e. `example.py`:

`MacOS`
```sh
pbpaste > example.py
```

Execute the example with python:
```
python example.py
```

## Incubed Modules


### Client
```python
Client(self,
chain: str = 'mainnet',
in3_config: ClientConfig = None,
cache_enabled: bool = True,
transport=<function https_transport at 0x101898e60>)
```

Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
Once with the latest list at hand, the client can request any other on-chain information using the same scheme.

**Arguments**:

- `chain` _str_ - Ethereum chain to connect to. Defaults to mainnet. Options: 'mainnet', 'kovan', 'goerli', 'ewc'.
- `in3_config` _ClientConfig or str_ - (optional) Configuration for the client. If not provided, default is loaded.
- `cache_enabled` _bool_ - False will disable local storage caching.
- `transport` _function_ - Transport function for custom request routing. Defaults to https.
  

#### refresh_node_list
```python
Client.refresh_node_list()
```

Gets the list of Incubed nodes registered in the selected chain registry contract.

**Returns**:

- `node_list` _NodeList_ - List of registered in3 nodes and metadata.
  

#### config
```python
Client.config()
```

Client configuration dictionary.

**Returns**:

- `config` _dict_ - Client configuration keys and values.
  

#### ens_namehash
```python
Client.ens_namehash(domain_name: str)
```

Name format based on [EIP-137](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-137.md#name-syntax)

**Arguments**:

- `domain_name` - ENS supported domain. mydomain.ens, mydomain.xyz, etc

**Returns**:

- `node` _str_ - Formatted string referred as `node` in ENS documentation
  

#### ens_address
```python
Client.ens_address(domain_name: str, registry: str = None)
```

Resolves ENS domain name to what account that domain points to.

**Arguments**:

- `domain_name` - ENS supported domain. mydomain.ens, mydomain.xyz, etc
- `registry` - ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e

**Returns**:

- `address` _str_ - Ethereum address corresponding to what account that domain points to.
  

#### ens_owner
```python
Client.ens_owner(domain_name: str, registry: str = None)
```

Resolves ENS domain name to Ethereum address of domain owner.

**Arguments**:

- `domain_name` - ENS supported domain. i.e mydomain.eth
- `registry` - ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e

**Returns**:

- `owner_address` _str_ - Ethereum address corresponding to domain owner.
  

#### ens_resolver
```python
Client.ens_resolver(domain_name: str, registry: str = None)
```

Resolves ENS domain name to Smart-contract address of the resolver registered for that domain.

**Arguments**:

- `domain_name` - ENS supported domain. i.e mydomain.eth
- `registry` - ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e

**Returns**:

- `resolver_contract_address` _str_ - Smart-contract address of the resolver registered for that domain.
  

### ClientConfig
```python
ClientConfig(self,
chain_finality_threshold: int = None,
account_secret: str = None,
latest_block_stall: int = None,
node_signatures: int = None,
node_signature_consensus: int = None,
node_min_deposit: int = None,
node_list_auto_update: bool = None,
node_limit: int = None,
request_timeout: int = None,
request_retries: int = None,
response_proof_level: str = None,
response_includes_code: bool = None,
response_keep_proof: bool = None,
transport_binary_format: bool = None,
transport_ignore_tls: bool = None,
cached_blocks: int = None,
cached_code_bytes: int = None,
boot_weights: bool = None,
in3_registry: dict = None)
```

Determines the behavior of the in3 client, which chain to connect to and how to manage information security policies.

Considering integrity is guaranteed and confidentiality is not available on public blockchains, these settings will provide a balance between availability, and financial stake in case of repudiation.

The newer the block is, higher are the chances it gets repudiated by a fork in the chain. In3 nodes will decide individually to sign on repudiable information, reducing the availability. If the application needs the very latest block, consider using a calculated value in `node_signature_consensus` and set `node_signatures` to zero. This setting is as secure as a light-client.

The older the block gets, the highest is its availability because of the close-to-zero repudiation risk, but blocks older than circa one year are stored in Archive Nodes, expensive computers, so, despite of the low risk, there are not many nodes available with such information, and they must search for the requested block in its database, lowering the availability as well. If the application needs access to _old_ blocks, consider setting `request_timeout` and `request_retries` to accomodate the time the archive nodes take to fetch the inforamtion.

The verification policy enforces an extra step of security, adding a financial stake in case of repudiation or false/broken proof. For high security application, consider setting a calculated value in `node_min_deposit` and request as much signatures as necessary in `node_signatures`. Setting `chain_finality_threshold` high will guarantee non-repudiability.

**All args are Optional. Defaults connect to Ethereum main network with regular security levels.**

**Arguments**:

- `chain_finality_threshold` _int_ - Behavior depends on the chain consensus algorithm: POA - percent of signers needed in order reach finality (% of the validators) i.e.: 60 %. POW - mined blocks on top of the requested, i.e. 8 blocks. Defaults are defined in enum.Chain.
- `latest_block_stall` _int_ - Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes. example: 10 - will ask for the state from (latestBlock-10).
- `account_secret` _str_ - Account SK to sign all in3 requests. (Experimental use `set_account_sk`) example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
- `node_signatures` _int_ - Node signatures attesting the response to your request. Will send a separate request for each. example: 3 nodes will have to sign the response.
- `node_signature_consensus` _int_ - Useful when node_signatures <= 1. The client will check for consensus in responses. example: 10 - will ask for 10 different nodes and compare results looking for a consensus in the responses.
- `node_min_deposit` _int_ - Only nodes owning at least this amount will be chosen to sign responses to your requests. i.e. 1000000000000000000 Wei
- `node_list_auto_update` _bool_ - If true the nodelist will be automatically updated. False may compromise data security.
- `node_limit` _int_ - Limit nodes stored in the client. example: 150 nodes
- `request_timeout` _int_ - Milliseconds before a request times out. example: 100000 ms
- `request_retries` _int_ - Maximum times the client will retry to contact a certain node. example: 10 retries
- `response_proof_level` _str_ - 'none'|'standard'|'full' Full gets the whole block Patricia-Merkle-Tree, Standard only verifies the specific tree branch concerning the request, None only verifies the root hashes, like a light-client does.
- `response_includes_code` _bool_ - If true, every request with the address field will include the data, if existent, that is stored in that wallet/smart-contract. If false, only the code digest is included.
- `response_keep_proof` _bool_ - If true, proof data will be kept in every rpc response. False will remove this data after using it to verify the responses. Useful for debugging and manually verifying the proofs.
- `transport_binary_format` - If true, the client will communicate with the server using a binary payload instead of json.
- `transport_ignore_tls` - The client usually verify https tls certificates. To communicate over insecure http, turn this on.
- `cached_blocks` _int_ - Maximum blocks kept in memory. example: 100 last requested blocks
- `cached_code_bytes` _int_ - Maximum number of bytes used to cache EVM code in memory. example: 100000 bytes
- `boot_weights` _bool_ - if true, the first request (updating the nodelist) will also fetch the current health status and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.
- `in3_registry` _dict_ - In3 Registry Smart Contract configuration data
  

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
- `address` _in3.Account_ - Address of the node, which is the public address it is signing with. example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
- `deposit` _int_ - Deposit of the node in wei example: 12350000
- `props` _int_ - Properties of the node. example: 3
- `timeout` _int_ - Time (in seconds) until an owner is able to receive his deposit back after he unregisters himself example: 3600
- `registerTime` _int_ - When the node was registered in (unixtime?)
- `weight` _int_ - Score based on qualitative metadata to base which nodes to ask signatures from.
  

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
  




### EthAccountApi
```python
EthAccountApi(self, runtime: In3Runtime, factory: EthObjectFactory)
```

Manages accounts and smart-contracts


#### create
```python
EthAccountApi.create(qrng=False)
```

Creates a new Ethereum account and saves it in the wallet.

**Arguments**:

- `qrng` _bool_ - True uses a Quantum Random Number Generator api for generating the private key.

**Returns**:

- `account` _Account_ - Newly created Ethereum account.
  

#### recover
```python
EthAccountApi.recover(secret: str)
```

Recovers an account from a secret.

**Arguments**:

- `secret` _str_ - Account private key in hexadecimal string

**Returns**:

- `account` _Account_ - Recovered Ethereum account.
  

#### parse_mnemonics
```python
EthAccountApi.parse_mnemonics(mnemonics: str)
```

Recovers an account secret from mnemonics phrase

**Arguments**:

- `mnemonics` _str_ - BIP39 mnemonics phrase.

**Returns**:

- `secret` _str_ - Account secret. Use `recover_account` to create a new account with this secret.
  

#### sign
```python
EthAccountApi.sign(private_key: str, message: str)
```

Use ECDSA to sign a message.

**Arguments**:

- `private_key` _str_ - Must be either an address(20 byte) or an raw private key (32 byte)"}}'
- `message` _str_ - Data to be hashed and signed. Dont input hashed data unless you know what you are doing.

**Returns**:

- `signed_message` _str_ - ECDSA calculated r, s, and parity v, concatenated. v = 27 + (r % 2)
  

#### balance
```python
EthAccountApi.balance(address: str, at_block: int = 'latest')
```

Returns the balance of the account of given address.

**Arguments**:

- `address` _str_ - address to check for balance
- `at_block` _int or str_ - block number IN3BlockNumber  or EnumBlockStatus

**Returns**:

- `balance` _int_ - integer of the current balance in wei.
  

#### send_transaction
```python
EthAccountApi.send_transaction(sender: Account,
transaction: NewTransaction)
```

Signs and sends the assigned transaction. Requires `account.secret` value set.
Transactions change the state of an account, just the balance, or additionally, the storage and the code.
Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the
gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the
latest block.

**Arguments**:

- `sender` _Account_ - Sender Ethereum account. Senders generally pay the gas costs, so they must have enough balance to pay gas + amount sent, if any.
- `transaction` _NewTransaction_ - All information needed to perform a transaction. Minimum is to and value. Client will add the other required fields, gas and chaindId.

**Returns**:

- `tx_hash` _hex_ - Transaction hash, used to get the receipt and check if the transaction was mined.
  

#### send_raw_transaction
```python
EthAccountApi.send_raw_transaction(signed_transaction: str)
```

Sends a signed and encoded transaction.

**Arguments**:

- `signed_transaction` - Signed keccak hash of the serialized transaction
  Client will add the other required fields, gas and chaindId.

**Returns**:

- `tx_hash` _hex_ - Transaction hash, used to get the receipt and check if the transaction was mined.
  

#### estimate_gas
```python
EthAccountApi.estimate_gas(transaction: NewTransaction)
```

Gas estimation for transaction. Used to fill transaction.gas field. Check RawTransaction docs for more on gas.

**Arguments**:

- `transaction` - Unsent transaction to be estimated. Important that the fields data or/and value are filled in.

**Returns**:

- `gas` _int_ - Calculated gas in Wei.
  

#### transaction_count
```python
EthAccountApi.transaction_count(address: str, at_block: int = 'latest')
```

Number of transactions mined from this address. Used to set transaction nonce.
Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
It exists to mitigate replay attacks.

**Arguments**:

- `address` _str_ - Ethereum account address
- `at_block` _int_ - Block number

**Returns**:

- `tx_count` _int_ - Number of transactions mined from this address.
  

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
  




### EthContractApi
```python
EthContractApi(self, runtime: In3Runtime, factory: EthObjectFactory)
```

Manages smart-contract data and transactions


#### call
```python
EthContractApi.call(transaction: NewTransaction,
block_number: int = 'latest')
```

Calls a smart-contract method. Will be executed locally by Incubed's EVM or signed and sent over to save the state changes.
Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.

**Arguments**:

  transaction (NewTransaction):
- `block_number` _int or str_ - Desired block number integer or 'latest', 'earliest', 'pending'.

**Returns**:

- `method_returned_value` - A hexadecimal. For decoding use in3.abi_decode.
  

#### storage_at
```python
EthContractApi.storage_at(address: str,
position: int = 0,
at_block: int = 'latest')
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
  

#### code
```python
EthContractApi.code(address: str, at_block: int = 'latest')
```

Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.

**Arguments**:

- `address` _str_ - Ethereum account address
- `at_block` _int or str_ - Block number

**Returns**:

- `bytecode` _str_ - Smart-Contract bytecode in hexadecimal.
  

#### encode
```python
EthContractApi.encode(fn_signature: str, *fn_args)
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
  

#### decode
```python
EthContractApi.decode(fn_signature: str, encoded_value: str)
```

Smart-contract ABI decoder. Used to parse rpc responses from the EVM.
Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)

**Arguments**:

- `fn_signature` - Function signature. e.g. `(address,string,uint256)` or `getBalance(address):uint256`.
  In case of the latter, the function signature will be ignored and only the return types will be parsed.
- `encoded_value` - Abi encoded values. Usually the string returned from a rpc to the EVM.

**Returns**:

- `decoded_return_values` _tuple_ - "0x1234567890123456789012345678901234567890", "0x05"
  




### EthereumApi
```python
EthereumApi(self, runtime: In3Runtime)
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
  

#### block_by_hash
```python
EthereumApi.block_by_hash(block_hash: str, get_full_block: bool = False)
```

Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined (get_block_by_number).

**Arguments**:

- `block_hash` _str_ - Desired block hash
- `get_full_block` _bool_ - If true, returns the full transaction objects, otherwise only its hashes.

**Returns**:

- `block` _Block_ - Desired block, if exists.
  

#### block_by_number
```python
EthereumApi.block_by_number(block_number: [<class 'int'>],
get_full_block: bool = False)
```

Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree (this) (get_block_by_hash).

**Arguments**:

- `block_number` _int or str_ - Desired block number integer or 'latest', 'earliest', 'pending'.
- `get_full_block` _bool_ - If true, returns the full transaction objects, otherwise only its hashes.

**Returns**:

- `block` _Block_ - Desired block, if exists.
  

#### transaction_by_hash
```python
EthereumApi.transaction_by_hash(tx_hash: str)
```

Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.

**Arguments**:

- `tx_hash` - Transaction hash.

**Returns**:

- `transaction` - Desired transaction, if exists.
  

#### transaction_receipt
```python
EthereumApi.transaction_receipt(tx_hash: str)
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

- `tx_receipt` - The mined Transaction data including event logs.
  

### Ethereum Objects


#### DataTransferObject
```python
DataTransferObject()
```

Maps marshalling objects transferred to, and from a remote facade, in this case, libin3 rpc api.
For more on design-patterns see [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.


#### Transaction
```python
Transaction(self, From: str, to: str, gas: int, gasPrice: int, hash: str,
nonce: int, transactionIndex: int, blockHash: str,
value: int, input: str, publicKey: str, standardV: int,
raw: str, creates: str, chainId: int, r: int, s: int,
v: int)
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
- `blockHash` _int_ - Block number that this transaction was mined in. null when its pending.
- `transactionIndex` _int_ - Integer of the transactions index position in the block. null when its pending.
- `signature` _hex str_ - ECDSA of transaction.data, calculated r, s and v concatenated. V is parity set by v = 27 + (r % 2).
  

#### NewTransaction
```python
NewTransaction(self,
From: str = None,
to: str = None,
nonce: int = None,
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


#### Log
```python
Log(self, address: <built-in function hex>,
blockHash: <built-in function hex>, blockNumber: int,
data: <built-in function hex>, logIndex: int, removed: bool,
topics: [<built-in function hex>],
transactionHash: <built-in function hex>, transactionIndex: int,
transactionLogIndex: int, Type: str)
```

Transaction Log for events and data returned from smart-contract method calls.


#### TransactionReceipt
```python
TransactionReceipt(self,
blockHash: <built-in function hex>,
blockNumber: int,
cumulativeGasUsed: int,
From: str,
gasUsed: int,
logsBloom: <built-in function hex>,
status: int,
transactionHash: <built-in function hex>,
transactionIndex: int,
logs: [<class 'in3.eth.model.Log'>] = None,
to: str = None,
contractAddress: str = None)
```

Receipt from a mined transaction.

**Arguments**:

  blockHash:
  blockNumber:
- `cumulativeGasUsed` - total amount of gas used by block
  From:
- `gasUsed` - amount of gas used by this specific transaction
  logs:
  logsBloom:
- `status` - 1 if transaction succeeded, 0 otherwise.
  transactionHash:
  transactionIndex:
- `to` - Account to which this transaction was sent. If the transaction was a contract creation this value is set to None.
- `contractAddress` - Contract Account address created, f the transaction was a contract creation, or None otherwise.
  

#### Account
```python
Account(self,
address: str,
chain_id: int,
secret: int = None,
domain: str = None)
```

An Ethereum account.

**Arguments**:

- `address` - Account address. Derived from public key.
- `chain_id` - ID of the chain the account is used in.
- `secret` - Account private key. A 256 bit number.
- `domain` - ENS Domain name. ie. niceguy.eth
  

## Library Runtime

Shared Library Runtime module

Loads `libin3` according to host hardware architecture and OS.
Maps symbols, methods and types from the library.
Encapsulates low-level rpc calls into a comprehensive runtime.


### In3Runtime
```python
In3Runtime(self, chain_id: int, cache_enabled: bool, transport_fn)
```

Instantiate libin3 and frees it when garbage collected.

**Arguments**:

- `chain_id` _int_ - Chain-id based on EIP-155. Default is 0x1 for Ethereum mainNet.
- `cache_enabled` _bool_ - False will disable local storage cache.
- `transport_fn` - Transport function to handle the HTTP Incubed Network requests.
  

### in3.libin3.rpc_api

Load libin3 shared library for the current system, map function ABI, sets in3 network transport functions.


#### libin3_new
```python
libin3_new(chain_id: int, cache_enabled: bool,
transport_fn: <function CFUNCTYPE at 0x10182ce60>)
```

Instantiate new In3 Client instance.

**Arguments**:

- `chain_id` _int_ - Chain id as integer
- `cache_enabled` _bool_ - False will disable local storage cache.
- `transport_fn` - Transport function for the in3 network requests
- `storage_fn` - Cache Storage function for node list and requests caching

**Returns**:

- `instance` _int_ - Memory address of the client instance, return value from libin3_new
  

#### libin3_free
```python
libin3_free(instance: int)
```

Free In3 Client objects from memory.

**Arguments**:

- `instance` _int_ - Memory address of the client instance, return value from libin3_new
  

#### libin3_call
```python
libin3_call(instance: int, fn_name: bytes, fn_args: bytes)
```

Make Remote Procedure Call to an arbitrary method of a libin3 instance

**Arguments**:

- `instance` _int_ - Memory address of the client instance, return value from libin3_new
- `fn_name` _bytes_ - Name of function that will be called in the client rpc.
- `fn_args` - (bytes) Serialized list of arguments, matching the parameters order of this function. i.e. ['0x123']

**Returns**:

- `result` _int_ - Function execution status.
  

#### libin3_set_pk
```python
libin3_set_pk(instance: int, private_key: bytes)
```

Register the signer module in the In3 Client instance, with selected private key loaded in memory.

**Arguments**:

- `instance` _int_ - Memory address of the client instance, return value from libin3_new
- `private_key` - 256 bit number.
  

#### libin3_in3_req_add_response
```python
libin3_in3_req_add_response(*args)
```

Transport function that registers a response to a request.

**Arguments**:

  *args:
  

#### libin3_new_bytes_t
```python
libin3_new_bytes_t(value: bytes, length: int)
```

C Bytes struct

**Arguments**:

- `length` - byte array length
- `value` - byte array

**Returns**:

- `ptr_addr` - address of the instance of this struct
  
