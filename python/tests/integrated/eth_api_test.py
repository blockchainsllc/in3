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

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_transaction_count(self):
        rpc = self.client.eth.get_transaction_count('0x6FA33809667A99A805b610C49EE2042863b1bb83')
        self.assertGreater(rpc, 0)

    def test_get_code(self):
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)

    def test_eth_call(self):
        # TODO
        transaction = in3.eth.NewTransaction(From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
                                             to="0x7ceabea4AA352b10fBCa48e6E8015bC73687ABD4",
                                             data="0xa9c70686",
                                             nonce=5)
        rpc = self.client.eth.eth_call(transaction)
        self.assertIsInstance(rpc, str)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(int(hex(0x97a142), 16))
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        # TODO
        tx_hash = '0xf1fe98a8a0582f9488953f8d1eae1564e56a1952d3208320ba52bfec3fd6de7a'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)


class EthereumGoerliTest(EthereumTest):

    def setUp(self):
        self.client = in3.Client('goerli', in3_config=mock_config)
        # self.client = in3.Client('goerli', in3_config=mock_config, transport=mock_transport)

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_code(self):
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)

    def test_get_transaction_count(self):
        rpc = self.client.eth.get_transaction_count('0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308')
        self.assertGreaterEqual(rpc, 0)

    def test_get_block_by_number(self):
        block_number = self.client.eth.block_number() - 15
        block = self.client.eth.get_block_by_number(block_number)
        self.assertIsInstance(block, in3.eth.Block)
        self.assertEqual(block_number, block.number)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)


class EthereumKovanTest(EthereumTest):

    def setUp(self):
        self.client = in3.Client('kovan', in3_config=mock_config)
        # self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_code(self):
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)

    def test_get_transaction_count(self):
        rpc = self.client.eth.get_transaction_count('0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308')
        self.assertGreaterEqual(rpc, 0)

    def test_get_block_by_number(self):
        block_number = self.client.eth.block_number() - 15
        block = self.client.eth.get_block_by_number(block_number)
        self.assertIsInstance(block, in3.eth.Block)
        self.assertEqual(block_number, block.number)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x561438bacbd058aca597dd8ebaafbf05df993c83c3224301f33d569c417d0db4'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)


if __name__ == '__main__':
    unittest.main()
