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
