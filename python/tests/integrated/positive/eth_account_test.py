"""
Integrated tests for `in3.eth.account` module.
"""
import unittest

import in3
from tests.integrated.mock.config import goerli_mock_config, mainchain_mock_config
from tests.integrated.mock.transport import mock_transport


class EthAccountGoerliTestCase(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mock_config)
        self.client = in3.Client('goerli', in3_config=goerli_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_checksum_address(self):
        missing_0x_address = '1fe2e9bf29AA1938859aF64C413361227d04059A'
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.eth.account.checksum_address(missing_0x_address)
        short_address = '0x1fe2e9bf29AA1938859aF64C413361227d04059'
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.eth.account.checksum_address(short_address)
        mixed_case_address = '0x1fe2e9bf29AA1938859aF64C413361227d04059A'
        address_false = self.client.eth.account.checksum_address(mixed_case_address, False)
        self.assertEqual(address_false, '0x1Fe2E9bf29aa1938859Af64C413361227d04059a')
        address_true = self.client.eth.account.checksum_address(mixed_case_address)
        self.assertNotEqual(address_true, '0x1Fe2E9bf29aa1938859Af64C413361227d04059a')
        # Check lower
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower()), address_true)
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower(), False), address_false)
        # Check immutable
        self.assertEqual(address_true, self.client.eth.account.checksum_address(address_true))
        self.assertEqual(address_false, self.client.eth.account.checksum_address(address_true, False))

    def test_sign(self):
        message = '0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
        private_key = '0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8'
        signature = self.client.eth.account.sign(private_key, message)
        self.assertEqual(signature,
                         "0xdd34194276b13f44d3f83401f87da6672dd8dc5905590b7ef44623f424af462f70ce23d37b4" +
                         "ada4ff33f5645df62524402c4fb5cac5dca3bce60331f8d6bc5d41c")

    def test_estimate_gas(self):
        transaction = in3.eth.NewTransaction(
            From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
            to="0xF5FEb05CA1b451d60b343f5Ee12df2Cc4ce2691B",
            nonce=5,
            data="0x9a5c90aa0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" +
                 "000000000000000000000000002386f26fc1000000000000000000000000000015094fc63f03e46fa211eb5708088dda62" +
                 "782dc4000000000000000000000000000000000000000000000004563918244f400000")
        gas = self.client.eth.account.estimate_gas(transaction)
        self.assertGreater(gas, 1000)

    def test_get_transaction_count(self):
        rpc = self.client.eth.account.transaction_count('0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308')
        self.assertGreaterEqual(rpc, 0)

    def test_get_balance(self):
        result = self.client.eth.account.balance('0x6FA33809667A99A805b610C49EE2042863b1bb83')
        self.assertGreaterEqual(result, 0)

    def test_send_tx(self):
        # 1000000000000000000 == 1 ETH
        # SK to PK 0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308
        secret = hex(0x9852782BEAD36C64161665586D33391ECEC1CCED7432A1D66FD326D38EA0171F)
        sender = self.client.eth.account.recover(secret)
        receiver = hex(0x6FA33809667A99A805b610C49EE2042863b1bb83)
        tx = in3.eth.NewTransaction(to=receiver, value=1463926659)
        tx_hash = self.client.eth.account.send_transaction(sender, tx)
        self.assertEqual(tx_hash, '0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd')

    def test_send_raw_transaction(self):
        raw_tx = "0xf867078449504f80825208946fa33809667a99a805b610c49ee2042863b1bb83845741bf83802ea0196e11e25ec9de" + \
                 "57108c7964753a034bb8f1dc02efdd1286b8dd865d21216dafa020ef4407a6d1c5f335ce5685cb49a039f704e32d1393" + \
                 "03576956ca5aaefc7ac3"
        tx_hash = self.client.eth.account.send_raw_transaction(raw_tx)
        self.assertEqual(tx_hash, "0x4456152b5f25509a9f6a4117205700f3b480cd837c855602bce6088a10c2fddd")


class EthAccountTestCase(EthAccountGoerliTestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_get_transaction_count(self):
        rpc = self.client.eth.account.transaction_count('0x6FA33809667A99A805b610C49EE2042863b1bb83')
        self.assertGreater(rpc, 0)

    def test_send_tx(self):
        # 1000000000000000000 == 1 ETH
        # SK to PK 0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308
        secret = hex(0xAC6D6BF94AD0AC65869EF6A0A47A9F2A201956D4AF3FFBE9DFE679399DACD3D9)
        sender = self.client.eth.account.recover(secret)
        receiver = hex(0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308)
        tx = in3.eth.NewTransaction(to=receiver, value=1463926659)
        tx_hash = self.client.eth.account.send_transaction(sender, tx)
        self.assertEqual(tx_hash, '0xb13b9d38642216af2545f1b9f882413bcdef13bec21def57c699d3a967d763bc')

    def test_send_raw_transaction(self):
        raw_tx = "0xf868098502540be400825208940b56ae81586d2728ceaf7c00a6020c5d63f02308845741bf838026a070af62c71480" + \
                 "8c7607ebe2cc756f20bed9790e5f5f20b192a83f689157a44bd0a0768a5d94766f1645e652dbef3826eed94074b5481a" + \
                 "453ad12aa52ae6f8e42606"
        tx_hash = self.client.eth.account.send_raw_transaction(raw_tx)
        self.assertEqual(tx_hash, "0xb13b9d38642216af2545f1b9f882413bcdef13bec21def57c699d3a967d763bc")
