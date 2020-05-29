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
        'avgBlockTime': 15},
    '0x5': {
        'contract': '0x5f51e413581dd76759e9eed51e63d14c8d1379c8',
        'registryId': '0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea',
        'needsUpdate': False,
        'avgBlockTime': 15},
    '0x2a': {
        'contract': '0x4c396dcf50ac396e5fdea18163251699b5fcca25',
        'registryId': '0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf',
        'needsUpdate': False,
        'avgBlockTime': 6},
}

mock_config = ClientConfig(node_list_auto_update=False,
                           node_signature_consensus=2,
                           node_signatures=2,
                           request_retries=1,
                           in3_registry=_registry,
                           cached_blocks=0,
                           cached_code_bytes=0)
