from in3.eth.factory import EthObjectFactory
from in3.eth.model import RawTransaction
from in3.libin3.runtime import In3Runtime
from in3.libin3.enum import In3Methods
from in3.eth.api import EthereumApi
from in3.model import In3Node, NodeList, ClientConfig


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

    def _configure(self, in3_config: ClientConfig):
        return self._runtime.call(In3Methods.CONFIG, in3_config.serialize())

    def node_list(self) -> NodeList:
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
        # TODO: Smart-Contract Api
        return self._runtime.call(In3Methods.ABI_ENCODE, fn_signature, fn_args)

    def abi_decode(self, fn_return_types: str, encoded_values: str) -> tuple:
        """
        Smart-contract ABI decoder. Used to parse rpc responses from the EVM.
        Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
        Args:
            fn_return_types: Function return types. e.g. `uint256`, `(address,string,uint256)` or `getBalance(address):uint256`.
            In case of the latter, the function signature will be ignored and only the return types will be parsed.
            encoded_values: Abi encoded values. Usually the string returned from a rpc to the EVM.
        Returns:
            decoded_return_values (tuple):  "0x1234567890123456789012345678901234567890", "0x05"
        """
        # TODO: Smart-Contract Api
        return self._runtime.call(In3Methods.ABI_DECODE, fn_return_types, encoded_values)

    def call(self, transaction: RawTransaction, block_number: int or str) -> int or str:
        """
        Calls a smart-contract method that does not store the computation. Will be executed locally by Incubed's EVM.
        curl localhost:8545 -X POST --data '{"jsonrpc":"2.0", "method":"eth_call", "params":[{"from": "eth.accounts[0]", "to": "0x65da172d668fbaeb1f60e206204c2327400665fd", "data": "0x6ffa1caa0000000000000000000000000000000000000000000000000000000000000005"}, "latest"], "id":1}'
        Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.
        Args:
            transaction (RawTransaction):
            block_number (int or str):  Desired block number integer or 'latest', 'earliest', 'pending'.
        Returns:
            method_returned_value: A hexadecimal. For decoding use in3.abi_decode.
        """
        # different than eth_call
        # eth_call_fn(c, contract, BLKNUM_LATEST(), "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));
        return self._runtime.call(In3Methods.CALL, transaction, block_number)

    # TODO add eth_set_pk_signer
    # TODO add sign_tx


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
