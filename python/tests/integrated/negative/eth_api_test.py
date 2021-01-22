"""
Integrated negative tests for `in3.eth` module.
"""
import unittest

import in3
from tests.integrated.mock.config import mainchain_mock_config, goerli_mock_config
from tests.integrated.mock.transport import mock_transport


class EthereumNegativeTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_get_block_by_number_client(self):
        for i in range(50):
            with self.assertRaises(in3.ClientException):
                self.client.eth.block_by_number(9937219)
            with self.assertRaises(in3.ClientException):
                self.client.eth.block_by_number(True)

    def test_get_transaction_by_hash_client(self):
        tx_bad_hash = '0xTe25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668aeP'
        tx_wrong_hash = '0xTe25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668aeb'
        tx_evil_hash = '0xa™a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668aeP'
        for i in range(20):
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_by_hash(tx_bad_hash)
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_by_hash(tx_wrong_hash)
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_by_hash(tx_evil_hash)

    def test_get_transaction_receipt_client(self):
        tx_bad_hash = '0xb13b9d38642216af2545f1b9f882413bcdef13bec21def57c699d3a967d763bP'
        tx_wrong_hash = '0xb13b9d38642216af2545f1b9f882413bcdef13bec21def57c699d3a967d763bb'
        tx_evil_hash = '0xa™a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668aeP'
        for i in range(20):
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_receipt(tx_bad_hash)
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_receipt(tx_wrong_hash)
            with self.assertRaises(in3.ClientException):
                self.client.eth.transaction_receipt(tx_evil_hash)


class ParsingTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_get_block_by_number(self):
        with self.assertRaises(AssertionError):
            self.client.eth.block_by_number(None)
        with self.assertRaises(AssertionError):
            self.client.eth.block_by_number(-1)
        with self.assertRaises(AssertionError):
            self.client.eth.block_by_number('œ∑´´†√¨')

    def test_get_transaction_by_hash(self):
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(None)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(-1)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash('œ∑´´†√¨')
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(True)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(False)
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae'
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(tx_hash)
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae77'
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_by_hash(tx_hash)

    def test_get_tx_receipt(self):
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(None)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(-1)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt('œ∑´´†√¨')
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(True)
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(False)
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae'
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(tx_hash)
        tx_hash = '0xae25a4b673bd87f40ea147a5506cb2ffb38e32ec1efc372c6730a5ba50668ae77'
        with self.assertRaises(in3.HashFormatException):
            self.client.eth.transaction_receipt(tx_hash)


class NegativeGoerliTest(EthereumNegativeTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=goerli_mock_config)
        self.client = in3.Client('goerli', in3_config=goerli_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)


if __name__ == '__main__':
    unittest.main()
