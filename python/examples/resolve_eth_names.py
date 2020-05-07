"""
Resolves ENS domains to Ethereum addresses
ENS is a smart-contract system that registers and resolves `.eth` domains.
"""
import in3


def _print():
    print('\nAddress for {} @ {}: {}'.format(domain, chain, address))
    print('Owner for {} @ {}: {}'.format(domain, chain, owner))


# Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
ens_address = '0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e'  # Available at Mainnet, Ropsten, Rinkeby and Goerli.
domain = 'depraz.eth'

print('\nEthereum Name Service')

# Instantiate In3 Client for Goerli
chain = 'goerli'
client = in3.Client(chain)
address = client.ens_resolve(domain, 'addr', ens_address)

# Instantiate In3 Client for Mainnet
chain = 'mainnet'
client = in3.Client(chain)
address = client.ens_resolve(domain, 'addr', ens_address)
owner = client.ens_resolve(domain, 'owner', ens_address)
_print()

# Instantiate In3 Client for Kovan
chain = 'kovan'
client = in3.Client(chain)
try:
    address = client.ens_resolve(domain, 'addr', ens_address)
    owner = client.ens_resolve(domain, 'owner', ens_address)
    _print()
except in3.ClientException:
    print('\nENS is not available on Kovan.')


# Produces
"""
Ethereum Name Service

Address for deprazz.eth @ goerli: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for deprazz.eth @ goerli: 0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308

Address for deprazz.eth @ mainnet: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for deprazz.eth @ mainnet: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308

ENS is not available on Kovan.
"""
