import unittest
import in3
from tests.config_mock import mock_config
from tests.transport import mock_transport


class EthereumTest(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client(in3_config=mock_config)
        # self.client = in3.Client(in3_config=mock_config, transport=mock_transport)

    def test_ethereum_sha3(self):
        digest = self.client.eth.keccak256('0x68656c6c6f20776f726c64')
        self.assertEqual(digest, '0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad')

    def test_eth_gasPrice(self):
        self.assertGreater(self.client.eth.gas_price(), 1000000)

    def test_block_number(self):
        result = self.client.eth.block_number()
        self.assertGreater(result, 1000)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(9937218)
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        # TODO
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae3'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        # TODO
        transaction = in3.eth.NewTransaction(From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
                                             to="0x7ceabea4AA352b10fBCa48e6E8015bC73687ABD4",
                                             data="0xa9c70686",
                                             nonce=5)
        rpc = self.client.eth.eth_call(transaction)
        self.assertIsInstance(rpc, str)

    def test_get_storage_at(self):
        # TODO
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_code(self):
        # TODO
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)


class EthereumGoerliTest(EthereumTest):

    def setUp(self):
        self.client = in3.Client('goerli', in3_config=mock_config)
        # self.client = in3.Client('goerli', in3_config=mock_config, transport=mock_transport)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(2581719)
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        # TODO
        transaction = in3.eth.NewTransaction(From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
                                             to="0x7ceabea4AA352b10fBCa48e6E8015bC73687ABD4",
                                             data="0xa9c70686",
                                             nonce=5)
        rpc = self.client.eth.eth_call(transaction)
        self.assertIsInstance(rpc, str)

    def test_get_storage_at(self):

        # TODO
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_code(self):

        # TODO
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)


class EthereumKovanTest(EthereumTest):

    def setUp(self):
        self.client = in3.Client('kovan', in3_config=mock_config)
        # self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(18135233)
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x561438bacbd058aca597dd8ebaafbf05df993c83c3224301f33d569c417d0db4'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        # TODO
        #
        # curl localhost:8545 -X POST --data '{"jsonrpc":"2.0", "method":"eth_call", "params":[{"from": "eth.accounts[0]", "to": "0x65da172d668fbaeb1f60e206204c2327400665fd", "data": "0x6ffa1caa0000000000000000000000000000000000000000000000000000000000000005"}, "latest"], "id":1}'
        transaction = in3.eth.NewTransaction(From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
                                             to="0x7ceabea4AA352b10fBCa48e6E8015bC73687ABD4",
                                             data="0xa9c70686",
                                             nonce=5)
        rpc = self.client.eth.eth_call(transaction)
        self.assertIsInstance(rpc, str)

    def test_get_storage_at(self):

        # TODO
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_code(self):

        # TODO
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)


if __name__ == '__main__':
    unittest.main()
