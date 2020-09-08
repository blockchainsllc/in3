"""
Manually calling ENS smart-contract
![UML Sequence Diagram of how Ethereum Name Service ENS resolves a name.](https://lh5.googleusercontent.com/_OPPzaxTxKggx9HuxloeWtK8ggEfIIBKRCEA6BKMwZdzAfUpIY6cz7NK5CFmiuw7TwknbhFNVRCJsswHLqkxUEJ5KdRzpeNbyg8_H9d2RZdG28kgipT64JyPZUP--bAizozaDcxCq34)
"""
import in3
from in3.model import Chain


client = in3.Client(Chain.GOERLI)
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
