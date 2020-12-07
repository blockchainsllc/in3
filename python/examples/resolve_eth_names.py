"""
Resolves ENS domains to Ethereum addresses
ENS is a smart-contract system that registers and resolves `.eth` domains.
"""
import in3


# Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
# Instantiate In3 Client for the Ethereum main net disabling cache to get the freshest address.
client = in3.Client(cache_enabled=False)
domain = 'depraz.eth'

if __name__ == '__main__':

    try:
        print('\nEthereum Name Service')
        address = client.ens_address(domain)
        owner = client.ens_owner(domain)
        print('\nAddress for {} @ {}: {}'.format(domain, 'mainnet', address))
        print('Owner for {} @ {}: {}'.format(domain, 'mainnet', owner))
    except in3.ClientException as e:
        print('Network might be unstable, try again later.\n Reason: ', str(e))


# Produces
"""
Ethereum Name Service

Address for depraz.eth @ mainnet: 0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for depraz.eth @ mainnet: 0x6fa33809667a99a805b610c49ee2042863b1bb83
"""
