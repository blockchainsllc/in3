"""
Integrated tests for `in3.eth` module.
"""
import unittest

import in3
from tests.integrated.mock.config import mainchain_mock_config, goerli_mock_config
from tests.integrated.mock.transport import mock_transport


class EthereumTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_ethereum_sha3(self):
        digest = self.client.eth.keccak256('0x68656c6c6f20776f726c64')
        self.assertEqual(digest, '0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad')
        digest = self.client.eth.keccak256('0x')
        self.assertEqual(digest, '0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470')
        self.assertIsNotNone(self.client.eth.keccak256('0x--'))
        self.assertIsNotNone(self.client.eth.keccak256('0x'))
        self.assertIsNotNone(self.client.eth.keccak256('0xmixed message'))
        self.assertIsNotNone(self.client.eth.keccak256('!@# 123 message 1234567890 !@#$%^&*()_+=-;'"[]{}`~"))
        self.assertIsNotNone(self.client.eth.keccak256('•¶¥¥•¶¥•¶¶§®¶®∫ˆ¨˜˜˜≥≤µ˜˜√ç≈Ωåß∂ƒ©˙∆˚¬…æ«‘“πøˆ¨¥†®´´œ'))
        self.assertIsNotNone(self.client.eth.keccak256(''))
        self.assertIsNotNone(self.client.eth.keccak256(123))
        self.assertIsNotNone(self.client.eth.keccak256(True))
        self.assertIsNotNone(self.client.eth.keccak256(None))

    def test_eth_gasPrice(self):
        self.assertGreater(self.client.eth.gas_price(), 1000000)

    def test_block_number(self):
        result = self.client.eth.block_number()
        self.assertGreater(result, 1000)

    def test_get_block_by_number(self):
        block = self.client.eth.block_by_number(9937218)
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('latest')
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('pending')
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('earliest')
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('laTest')
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('peNding')
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('eaRliest')
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae3'
        tx = self.client.eth.transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_get_tx_receipt(self):
        tx_hash = '0xb13b9d38642216af2545f1b9f882413bcdef13bec21def57c699d3a967d763bc'
        result = self.client.eth.transaction_receipt(tx_hash)
        self.assertIsInstance(result, in3.eth.TransactionReceipt)


class EthereumGoerliTest(EthereumTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=goerli_mock_config)
        self.client = in3.Client('goerli', in3_config=goerli_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_get_block_by_number(self):
        block = self.client.eth.block_by_number(2581719)
        self.assertIsInstance(block, in3.eth.Block)
        block = self.client.eth.block_by_number('latest')
        self.assertIsInstance(block, in3.eth.Block)

    def test_get_transaction_by_hash(self):
        tx_hash = '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd'
        tx = self.client.eth.transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

    def test_get_tx_receipt(self):
        tx_hash = '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd'
        result = self.client.eth.transaction_receipt(tx_hash)
        self.assertIsInstance(result, in3.eth.TransactionReceipt)


if __name__ == '__main__':
    unittest.main()
