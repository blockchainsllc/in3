"""
Connects to Ethereum and fetches attested information from each chain.
"""
import in3
from in3.eth.enums import Chain


print('\nEthereum Main Network')
client = in3.Client()
latest_block = client.eth.block_number()
gas_price = client.eth.gas_price()
print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))

print('\nEthereum Kovan Test Network')
client = in3.Client(Chain.KOVAN)
latest_block = client.eth.block_number()
gas_price = client.eth.gas_price()
print('Latest BN: {}\nGas Price: {} Wei'.format(latest_block, gas_price))

print('\nEthereum Goerli Test Network')
client = in3.Client(Chain.GOERLI)
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
