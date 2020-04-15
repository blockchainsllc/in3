import in3
import in3.model

print('\nEthereum Goerli Test Network')
client = in3.Client('goerli')
node_list = client.get_node_list()
print('\nIncubed Registry:')
print('\ttotal servers:', node_list.totalServers)
print('\tlast updated in block:', node_list.lastBlockNumber)
print('\tregistry ID:', node_list.registryId)
print('\tcontract address:', node_list.contract)
print('\nNodes Registered:\n')
for node in node_list.nodes:
    print('\turl:', node.url)
    print('\tdeposit:', node.deposit)
    print('\tweight:', node.weight)
    print('\tregistered in block:', node.registerTime)
    print('\n')

# Results Example
"""
Ethereum Goerli Test Network

Incubed Registry:
    total servers: 7
    last updated in block: 2320627
    registry ID: 0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea
    contract address: 0x5f51e413581dd76759e9eed51e63d14c8d1379c8

Nodes Registered:

    url: https://in3-v2.slock.it/goerli/nd-1
    deposit: 10000000000000000
    weight: 2000
    registered in block: 1576227711


    url: https://in3-v2.slock.it/goerli/nd-2
    deposit: 10000000000000000
    weight: 2000
    registered in block: 1576227741


    url: https://in3-v2.slock.it/goerli/nd-3
    deposit: 10000000000000000
    weight: 2000
    registered in block: 1576227801


    url: https://in3-v2.slock.it/goerli/nd-4
    deposit: 10000000000000000
    weight: 2000
    registered in block: 1576227831


    url: https://in3-v2.slock.it/goerli/nd-5
    deposit: 10000000000000000
    weight: 2000
    registered in block: 1576227876


    url: https://tincubeth.komputing.org/
    deposit: 10000000000000000
    weight: 1
    registered in block: 1578947320


    url: https://h5l45fkzz7oc3gmb.onion/
    deposit: 10000000000000000
    weight: 1
    registered in block: 1578954071
"""
