"""
Resolves ENS domains to Ethereum addresses
ENS is a smart-contract system that registers and resolves `.eth` domains.
"""
import in3
from in3.eth.enums import Chain


def _print():
    print('\nAddress for {} @ {}: {}'.format(domain, chain, address))
    print('Owner for {} @ {}: {}'.format(domain, chain, owner))


# Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
domain = 'depraz.eth'

print('\nEthereum Name Service')

# Instantiate In3 Client for Goerli
chain = Chain.GOERLI
client = in3.Client(chain, cache_enabled=False)
address = client.ens_address(domain)
# owner = client.ens_owner(domain)
# _print()

# Instantiate In3 Client for Mainnet
chain = Chain.MAINNET
client = in3.Client(chain, cache_enabled=False)
address = client.ens_address(domain)
owner = client.ens_owner(domain)
_print()

# Instantiate In3 Client for Kovan
chain = Chain.KOVAN
client = in3.Client(chain, cache_enabled=True)
try:
    address = client.ens_address(domain)
    owner = client.ens_owner(domain)
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
