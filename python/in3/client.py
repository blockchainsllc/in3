import re

from in3.eth.api import EthereumApi
from in3.eth.factory import EthObjectFactory
from in3.libin3.enum import In3Methods
from in3.libin3.runtime import In3Runtime
from in3.exception import EnsDomainFormatException
from in3.model import In3Node, NodeList, ClientConfig, chain_configs
from in3.transport import https_transport


class Client:
    """
    Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
    the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
    Once with the latest list at hand, the client can request any other on-chain information using the same scheme.
    Args:
        chain (str): Ethereum chain to connect to. Defaults to mainnet. Options: 'mainnet', 'goerli', 'ewc', 'btc', 'ipfs'.
        in3_config (ClientConfig or str): (optional) Configuration for the client. If not provided, default is loaded.
        transport (function): Transport function for custom request routing. Defaults to https.
        cache_enabled (bool): False will disable local storage caching.
        test_instance (bool): True will create a test instance of IN3. HIGH SECURITY RISK - USE FOR TESTS ONLY.
    """

    def __init__(self, chain: str = 'mainnet', in3_config: ClientConfig = None, transport=https_transport,
                 cache_enabled: bool = True, test_instance: bool = False):

        if not isinstance(chain, str) or chain.lower() not in ['mainnet', 'goerli', 'ewc', 'btc', 'ipfs']:
            raise AssertionError('Client: Chain name not supported. Try mainnet, evan, goerli, ewc, or ipfs.')
        # TODO: Clear Chain-configs
        if in3_config and not isinstance(in3_config, ClientConfig):
            raise AssertionError('Client: Use in3.ClientConfig to create a new client configuration instance.')

        self._runtime = In3Runtime(chain_configs[chain.lower()].chain_id, transport, cache_enabled, test_instance)
        if in3_config:
            self._configure(in3_config)
        self.eth = EthereumApi(self._runtime)
        self._factory = In3ObjectFactory(self._runtime)

    def _configure(self, in3_config: ClientConfig) -> bool:
        """
        Send RPC to change client configuration. Don't use outside the constructor, might cause instability.
        """
        fn_args = str([in3_config.serialize()]).replace('\'', '')
        return self._runtime.call(In3Methods.CONFIGURE, fn_args, formatted=True)

    def refresh_node_list(self) -> NodeList:
        """
        Gets the list of Incubed nodes registered in the selected chain registry contract.
        Returns:
            node_list (NodeList): List of registered in3 nodes and metadata.
        """
        node_list_dict = self._runtime.call(In3Methods.NODE_LIST)
        return self._factory.get_node_list(node_list_dict)

    def config(self) -> dict:
        """
        Client configuration dictionary.
        Returns:
            config (dict): Client configuration keys and values.
        """
        # TODO: Marshalling
        return self._runtime.call(In3Methods.GET_CONFIG)

    def ens_namehash(self, domain_name: str) -> str:
        """
        Name format based on [EIP-137](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-137.md#name-syntax)
        Args:
            domain_name: ENS supported domain. mydomain.ens, mydomain.xyz, etc
        Returns:
            node (str): Formatted string referred as `node` in ENS documentation
        """
        if not isinstance(domain_name, str) or not re.match(r'(\w+.eth$)', domain_name):
            raise EnsDomainFormatException()
        return self._runtime.call(In3Methods.ENSRESOLVE, domain_name, 'hash')

    def ens_address(self, domain_name: str, registry: str = None) -> str:
        """
        Resolves ENS domain name to what account that domain points to.
        Args:
            domain_name: ENS supported domain. mydomain.ens, mydomain.xyz, etc
            registry: ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e
        Returns:
            address (str): Ethereum address corresponding to what account that domain points to.
        """
        # TODO: Add handlers to Account
        # TODO: Add serialization in account factory
        if registry:
            registry = self._factory.get_address(registry)
        if not isinstance(domain_name, str) or not re.match(r'(\w+.eth$)', domain_name):
            raise EnsDomainFormatException()
        return self._runtime.call(In3Methods.ENSRESOLVE, domain_name, 'addr', registry)

    def ens_owner(self, domain_name: str, registry: str = None) -> str:
        """
        Resolves ENS domain name to Ethereum address of domain owner.
        Args:
            domain_name: ENS supported domain. i.e mydomain.eth
            registry: ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e
        Returns:
            owner_address (str): Ethereum address corresponding to domain owner.
        """
        if registry:
            registry = self._factory.get_address(registry)
        if not isinstance(domain_name, str) or not re.match(r'(\w+.eth$)', domain_name):
            raise EnsDomainFormatException()
        return self._runtime.call(In3Methods.ENSRESOLVE, domain_name, 'owner', registry)

    def ens_resolver(self, domain_name: str, registry: str = None) -> str:
        """
        Resolves ENS domain name to Smart-contract address of the resolver registered for that domain.
        Args:
            domain_name: ENS supported domain. i.e mydomain.eth
            registry: ENS registry contract address. i.e. 0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e
        Returns:
            resolver_contract_address (str): Smart-contract address of the resolver registered for that domain.
        """
        if registry:
            registry = self._factory.get_address(registry)
        if not isinstance(domain_name, str) or not re.match(r'(\w+.eth$)', domain_name):
            raise EnsDomainFormatException()
        return self._runtime.call(In3Methods.ENSRESOLVE, domain_name, 'resolver', registry)


class In3ObjectFactory(EthObjectFactory):

    # TODO: Decode abi for props
    # TODO: Convert registerTime to human readable
    def get_node_list(self, serialized: dict) -> NodeList:
        mapping = {
            "nodes": list,
            "contract": self.get_account,
            "registryId": str,
            "lastBlockNumber": int,
            "totalServers": int,
        }
        deserialized_dict = self._deserialize(serialized, mapping)
        nodes = [self.get_in3node(node) for node in deserialized_dict['nodes']]
        deserialized_dict['nodes'] = nodes
        return NodeList(**deserialized_dict)

    def get_in3node(self, serialized: dict) -> In3Node:
        mapping = {
            "url": str,
            "address": self.get_account,
            "index": int,
            "deposit": self.get_integer,
            "props": str,
            "timeout": int,
            "registerTime": int,
            "weight": int
        }
        deserialized_dict = self._deserialize(serialized, mapping)
        return In3Node(**deserialized_dict)
