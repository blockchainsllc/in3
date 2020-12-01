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
