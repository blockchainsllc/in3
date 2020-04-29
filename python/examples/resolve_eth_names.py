if __name__ == '__main__':

    import in3

    print('\nEthereum Name Service\n')
    # Instantiate the In3 Client
    client = in3.Client('goerli')

    # Find ENS for the desired chain or the address of your own ENS resolver. https://docs.ens.domains/ens-deployments
    ens_address = '0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e'  # Available at Mainnet, Ropsten, Rinkeby and Goerli.
    address = client.ens_resolve('depraz.eth', 'addr', ens_address)
    print('Address for deprazz.eth@goerli: ', address)

    # Same can be achieved by making an eth_call to the ENS smart-contract
    tx = {
        "to": ens_address,
        "data": '0x02571be34a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e'
    }
    transaction = in3.eth.NewTransaction(**tx)
    owner = client.eth.contract.eth_call(transaction)
    print('Owner for deprazz.eth@goerli: ', owner)

# Results Example
"""
Ethereum Name Service

Address for deprazz.eth@goerli:  0x0b56ae81586d2728ceaf7c00a6020c5d63f02308
Owner for deprazz.eth@goerli:  0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308
"""
