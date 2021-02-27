"""
Configuration for `in3.client.Client`. Enables mocked in3 network responses to be valid.
Disable node list updates, and collect signed responses from both boot nodes.
"""
from in3 import ClientConfig

_registry = {
    '0x1': {
        'contract': '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
        'registryId': '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb',
        'needsUpdate': False,
        'avgBlockTime': 15,
        'nodeList': [
            {'url': 'https://in3-v2.slock.it/mainnet/nd-1',
             'address': '0x45d45e6ff99e6c34a235d263965910298985fcfe',
             'props': 0xFFFF},
            {'url': 'https://in3-v2.slock.it/mainnet/nd-2',
             'address': '0x1fe2e9bf29aa1938859af64c413361227d04059a',
             'props': 0xFFFF},
        ]},
    '0x5': {
        'contract': '0x5f51e413581dd76759e9eed51e63d14c8d1379c8',
        'registryId': '0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea',
        'needsUpdate': False,
        'avgBlockTime': 15,
        'nodeList': [
            {'url': 'https://in3-v2.slock.it/goerli/nd-1',
             'address': '0x45d45e6ff99e6c34a235d263965910298985fcfe',
             'props': 0xFFFF},
            {'url': 'https://in3-v2.slock.it/goerli/nd-2',
             'address': '0x1fe2e9bf29aa1938859af64c413361227d04059a',
             'props': 0xFFFF},
        ]},
    '0x7d0': {
        'contract': '0xa93b57289070550c82edb1106e12bb37138948b8',
        'registryId': '0xf0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb69196',
        'needsUpdate': False,
        'nodeList': [
            {'url': 'https://in3-v2.slock.it/ipfs/nd-1',
             'address': '0x45d45e6ff99e6c34a235d263965910298985fcfe',
             'props': 0xFFFF},
            {'url': 'https://in3-v2.slock.it/ipfs/nd-2',
             'address': '0x1fe2e9bf29aa1938859af64c413361227d04059a',
             'props': 0xFFFF},
        ]},
    '0x99': {
        'contract': '0xed7bb275ca33c46ef3875a9c959c91553ca6acb8',
        'registryId': '0x084ec5cd9274e7c05b827a0d417f92820eb249b9d4ae6e497e355620114a52dc',
        'needsUpdate': False,
        'avgBlockTime': 600,
        'nodeList': [
            {'url': 'https://in3-v2.slock.it/btc/nd-1',
             'address': '0x45d45e6ff99e6c34a235d263965910298985fcfe',
             'props': 0xFFFF},
        ]},
}

mainchain_mock_config = ClientConfig(node_list_auto_update=False,
                                     # node_signature_consensus=2,
                                     # node_signatures=2,
                                     latest_block_stall=10,
                                     request_retries=1,
                                     in3_registry=_registry['0x1'])

goerli_mock_config = ClientConfig(node_list_auto_update=False,
                                  # node_signature_consensus=2,
                                  # node_signatures=2,
                                  latest_block_stall=6,
                                  request_retries=1,
                                  in3_registry=_registry['0x5'])

ipfs_mock_config = ClientConfig(node_list_auto_update=False,
                                # node_signature_consensus=2,
                                # node_signatures=2,
                                request_retries=1,
                                in3_registry=_registry['0x7d0'])

btc_mock_config = ClientConfig(node_list_auto_update=False,
                               node_signature_consensus=1,
                               node_signatures=1,
                               latest_block_stall=4,
                               request_retries=1,
                               in3_registry=_registry['0x99'])
