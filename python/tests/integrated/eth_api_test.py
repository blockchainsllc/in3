import unittest
import in3
from tests.config_mock import mock_config
from tests.transport import mock_transport


class EthereumTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mock_config)
        self.client = in3.Client(in3_config=mock_config, transport=mock_transport)

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
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae3'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        tx = {
            "to": hex(0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e),
            "data": hex(0x02571be34a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e)
        }
        transaction = in3.eth.NewTransaction(**tx)
        address = self.client.eth.eth_call(transaction)
        self.assertEqual(address, '0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308')

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0x3589d05a1ec4Af9f65b0E5554e645707775Ee43C", 1)
        self.assertEqual(storage, '0x000000000000000000000000000000000000000000000000000000647261676f')

    def test_get_code(self):
        code = self.client.eth.get_code("0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e")
        self.assertEqual(len(code), 10694)


class EthereumGoerliTest(EthereumTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mock_config)
        self.client = in3.Client('goerli', in3_config=mock_config, transport=mock_transport)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(2581719)
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        tx = {
            "to": hex(0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e),
            "data": hex(0x02571be34a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e)
        }
        transaction = in3.eth.NewTransaction(**tx)
        address = self.client.eth.eth_call(transaction)
        self.assertEqual(address, '0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308')

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0x4B1488B7a6B320d2D721406204aBc3eeAa9AD329", 1)
        self.assertEqual(storage, '0x0')

    def test_get_code(self):
        code = self.client.eth.get_code("0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e")
        self.assertEqual(len(code), 10694)


class EthereumKovanTest(EthereumTest):

    def setUp(self):
        # self.client = in3.Client('kovan', in3_config=mock_config)
        self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_get_block_by_number(self):
        block = self.client.eth.get_block_by_number(18135233)
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x561438bacbd058aca597dd8ebaafbf05df993c83c3224301f33d569c417d0db4'
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_eth_call(self):
        # TODO: Future
        return

    def test_get_storage_at(self):
        # TODO: Future
        return

    def test_get_code(self):
        # TODO: Future
        return


if __name__ == '__main__':
    unittest.main()
