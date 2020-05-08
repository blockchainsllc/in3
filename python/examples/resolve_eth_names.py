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
client = in3.Client(chain)
address = client.ens_resolve(domain)

# Instantiate In3 Client for Mainnet
chain = 'mainnet'
client = in3.Client(chain)
address = client.ens_resolve(domain)
owner = client.ens_resolve_owner(domain)
_print()

# Instantiate In3 Client for Kovan
chain = 'kovan'
client = in3.Client(chain)
try:
    address = client.ens_resolve(domain)
    owner = client.ens_resolve_owner(domain)
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
