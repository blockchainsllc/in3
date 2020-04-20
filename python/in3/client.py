from in3.eth.factory import EthObjectFactory
from in3.libin3.runtime import In3Runtime
from in3.libin3.enum import In3Methods
from in3.eth.api import EthereumApi
from in3.model import In3Node, NodeList, ClientConfig

import re


class Client:
    """
    Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
    the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
    Once with the latest list at hand, the client can request any other on-chain information using the same scheme.
    Args:
        in3_config (ClientConfig or str): (optional) Configuration for the client. If not provided, default is loaded.
    """

    def __init__(self, in3_config: ClientConfig or str = None):
        super().__init__()
        if isinstance(in3_config, ClientConfig):
            self.config = in3_config
        elif isinstance(in3_config, str):
            if in3_config not in ['mainnet', 'kovan', 'goerli', 'evan', 'ipfs']:
                raise ValueError('Chain name not supported. Try mainnet, kovan, goerli, evan, ipfs.')
            self.config = ClientConfig(chain_id=in3_config)
        else:
            self.config = ClientConfig()
        self._runtime = In3Runtime(self.config.timeout)
        self._configure(in3_config=self.config)
        self.eth = EthereumApi(runtime=self._runtime, chain_id=self.config.chainId)
        self._factory = In3ObjectFactory(self.eth.account.checksum_address, self.config.chainId)

    def _configure(self, in3_config: ClientConfig) -> bool:
        fn_args = str([in3_config.serialize()]).replace('\'', '')
        return self._runtime.call(In3Methods.CONFIG, fn_args, formatted=True)

    def get_node_list(self) -> NodeList:
        """
        Gets the list of Incubed nodes registered in the selected chain registry contract.
        Returns:
            node_list (NodeList): List of registered in3 nodes and metadata.
        """
        node_list_dict = self._runtime.call(In3Methods.IN3_NODE_LIST)
        return self._factory.get_node_list(node_list_dict)

    def abi_encode(self, fn_signature: str, *fn_args) -> str:
        """
        Smart-contract ABI encoder. Used to serialize a rpc to the EVM.
        Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
        Note: Parameters refers to the list of variables in a method declaration.
        Arguments are the actual values that are passed in when the method is invoked.
        When you invoke a method, the arguments used must match the declaration's parameters in type and order.
        Args:
            fn_signature (str): Function name, with parameters. i.e. `getBalance(uint256):uint256`, can contain the return types but will be ignored.
            fn_args (tuple): Function parameters, in the same order as in passed on to method_name.
        Returns:
            encoded_fn_call (str): i.e. "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890"
        """
        is_signature = re.match(r'.*(\(.+\))', fn_signature)
        _types = ["address", "string", "uint", "string", "bool", "bytes", "int"]
        contains_type = [_type for _type in _types if _type in fn_signature]
        if not is_signature or not contains_type:
            raise AssertionError('Function signature is not valid. A valid example is balanceOf(address).')
        return self._runtime.call(In3Methods.ABI_ENCODE, fn_signature, fn_args)

    def abi_decode(self, fn_signature: str, encoded_value: str) -> tuple:
        """
        Smart-contract ABI decoder. Used to parse rpc responses from the EVM.
        Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
        Args:
            fn_signature: Function signature. e.g. `(address,string,uint256)` or `getBalance(address):uint256`.
            In case of the latter, the function signature will be ignored and only the return types will be parsed.
            encoded_value: Abi encoded values. Usually the string returned from a rpc to the EVM.
        Returns:
            decoded_return_values (tuple):  "0x1234567890123456789012345678901234567890", "0x05"
        """
        if not encoded_value.startswith('0x'):
            raise AssertionError("Encoded values must start with 0x")
        if len(encoded_value[2:]) <= 64:
            raise AssertionError("Encoded values must be longer than 64 characters.")
        is_signature = re.match(r'.*(\(.+\))', fn_signature)
        _types = ["address", "string", "uint", "string", "bool", "bytes", "int"]
        contains_type = [_type for _type in _types if _type in fn_signature]
        if not is_signature or not contains_type:
            raise AssertionError('Function signature is not valid. A valid example is balanceOf(address).')
        return self._runtime.call(In3Methods.ABI_DECODE, fn_signature, encoded_value)

    # TODO add eth_set_pk_signer
    # TODO add sign_tx
    # TODO add ens
    # TODO add getConfig


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
