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
import in3.model
import in3

if __name__ == '__main__':

    print('\nEthereum Goerli Test Network')
    goerli_cfg = in3.ClientConfig(chain_id=str(in3.model.Chain.GOERLI), node_signatures=3, request_timeout=10000)
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
