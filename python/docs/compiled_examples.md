## Examples

### connect_to_ethereum

source : [in3-c/python/examples/connect_to_ethereum.py](https://github.com/blockchainsllc/in3/blob/master/python/examples/connect_to_ethereum.py)



```python
"""
Connects to Ethereum and fetches attested information from each chain.
"""
import in3

if __name__ == '__main__':

    client = in3.Client()
    try:
        print('\nEthereum Main Network')
        latest_block = client.eth.block_number()
        gas_price = client.eth.gas_price()
        print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))

    goerli_client = in3.Client('goerli')
    try:
        print('\nEthereum Goerli Test Network')
        latest_block = goerli_client.eth.block_number()
        gas_price = goerli_client.eth.gas_price()
        print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))

# Produces
"""
Ethereum Main Network
Latest BN: 9801135
Gas Price: 2000000000 Wei

Ethereum Goerli Test Network
Latest BN: 2460853
Gas Price: 4610612736 Wei
"""

```

### incubed_network

source : [in3-c/python/examples/incubed_network.py](https://github.com/blockchainsllc/in3/blob/master/python/examples/incubed_network.py)



```python
"""
Shows Incubed Network Nodes Stats
"""
import in3

if __name__ == '__main__':

    print('\nEthereum Goerli Test Network')
    client = in3.Client('goerli')
    try:
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
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))

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

source : [in3-c/python/examples/resolve_eth_names.py](https://github.com/blockchainsllc/in3/blob/master/python/examples/resolve_eth_names.py)



```python
"""
Resolves ENS domains to Ethereum addresses
ENS is a smart-contract system that registers and resolves `.eth` domains.
"""
import in3


# Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
# Instantiate In3 Client for the Ethereum main net disabling cache to get the freshest address.
client = in3.Client(cache_enabled=False)
domain = 'depraz.eth'

if __name__ == '__main__':

    try:
        print('\nEthereum Name Service')
        address = client.ens_address(domain)
        owner = client.ens_owner(domain)
        print('\nAddress for {} @ {}: {}'.format(domain, 'mainnet', address))
        print('Owner for {} @ {}: {}'.format(domain, 'mainnet', owner))
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))


# Produces
"""
Ethereum Name Service

Address for depraz.eth @ mainnet: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for depraz.eth @ mainnet: 0x6fa33809667a99a805b610c49ee2042863b1bb83
"""

```

### send_transaction

source : [in3-c/python/examples/send_transaction.py](https://github.com/blockchainsllc/in3/blob/master/python/examples/send_transaction.py)



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

if __name__ == '__main__':

    try:
        print('-= Ethereum Transaction using Incubed =- \n')
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

source : [in3-c/python/examples/smart_contract.py](https://github.com/blockchainsllc/in3/blob/master/python/examples/smart_contract.py)



```python
"""
Manually calling the ENS smart-contract
![UML Sequence Diagram of how Ethereum Name Service ENS resolves a name.](https://lh5.googleusercontent.com/_OPPzaxTxKggx9HuxloeWtK8ggEfIIBKRCEA6BKMwZdzAfUpIY6cz7NK5CFmiuw7TwknbhFNVRCJsswHLqkxUEJ5KdRzpeNbyg8_H9d2RZdG28kgipT64JyPZUP--bAizozaDcxCq34)
"""
import in3


client = in3.Client('goerli')

if __name__ == '__main__':

    try:
        print('-= Smart-Contract Call on Ethereum using Incubed =- \n')
        domain_name = client.ens_namehash('depraz.eth')
        ens_registry_addr = '0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e'
        ens_resolver_abi = 'resolver(bytes32):address'

        # Find resolver contract for ens name
        resolver_tx = {
            "to": ens_registry_addr,
            "data": client.eth.contract.encode(ens_resolver_abi, domain_name)
        }
        tx = in3.eth.NewTransaction(**resolver_tx)
    except in3.ClientException as e:
            print('Something went wrong converting data.\n Reason: ', str(e))
    try:
        # Make the smart contract call
        print('Calling the ENS registry contract.')
        encoded_resolver_addr = client.eth.contract.call(tx)
        resolver_address = client.eth.contract.decode(ens_resolver_abi, encoded_resolver_addr)
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))
    try:
        # Resolve name
        ens_addr_abi = 'addr(bytes32):address'
        name_tx = {
            "to": resolver_address,
            "data": client.eth.contract.encode(ens_addr_abi, domain_name)
        }
    except in3.ClientException as e:
        print('Something went wrong converting data.\n Reason: ', str(e))
    try:
        print('Calling the ENS resolver contract.')
        encoded_domain_address = client.eth.contract.call(in3.eth.NewTransaction(**name_tx))
        domain_address = client.eth.contract.decode(ens_addr_abi, encoded_domain_address)

        print('\nENS domain:\n{}\nResolved by:\n{}\nTo address:\n{}'.format(domain_name, resolver_address,
                                                                          domain_address))
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))


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
