import unittest
import in3
from tests.transport import mock_transport, mock_config


class UtilsTestCase(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client('goerli', in3_config=mock_config)
        config = self.client.get_config()
        # self.client = in3.Client('goerli', in3_config=mock_client_config, transport=mock_transport)
        pass

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
        self.assertEqual(address_true, '0x1fe2E9Bf29aa1938859Af64c413361227D04059a')
        # Check lower
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower()), address_true)
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower(), False), address_false)
        # Check immutable
        self.assertEqual(address_true, self.client.eth.account.checksum_address(address_true))
        self.assertEqual(address_false, self.client.eth.account.checksum_address(address_true, False))

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

    def test_get_tx_receipt(self):
        tx_hash = '0x738e8878228901ad8143cfcf908abb2d2044c83231a398731841be41970a79ce'
        result = self.client.eth.account.get_transaction_receipt(tx_hash)
        self.assertIsInstance(result, in3.eth.TransactionReceipt)

    def test_sign(self):
        message = '0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
        private_key = '0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8'
        singature = self.client.eth.account.sign(private_key, message)
        self.assertEqual(singature,
                         "0xdd34194276b13f44d3f83401f87da6672dd8dc5905590b7ef44623f424af462f70ce23d37b4" +
                         "ada4ff33f5645df62524402c4fb5cac5dca3bce60331f8d6bc5d41c")

    def test_send_tx(self):
        # 1000000000000000000 == 1 ETH
        # SK to PK 0x0b56Ae81586D2728Ceaf7C00A6020C5D63f02308
        secret = hex(0x9852782BEAD36C64161665586D33391ECEC1CCED7432A1D66FD326D38EA0171F)
        sender = self.client.wallet.recover_account('sender', secret)
        # sender_balance = self.client.eth.get_balance(sender.address)
        receiver = hex(0x6FA33809667A99A805b610C49EE2042863b1bb83)
        # receiver_balance = self.client.eth.get_balance(receiver)
        tx = in3.eth.NewTransaction(to=receiver, value=1463926659)
        tx_hash = self.client.eth.account.send_transaction(sender, tx)
        self.assertEqual(tx_hash, '0xe25bc84c719130ec49f40426b5aaeb922213d22c75cf3253aa6b3cdcfca85839')

    def test_send_raw_transaction(self):
        raw_tx = "0xf8670384b2d05e00825208946fa33809667a99a805b610c49ee2042863b1bb83845741bf83802da07c943a2d48f3ec" + \
                 "5ecadbd776564c23a238915f7ccbf195bf2c3e8d4041d6ac98a02afc1ae8a45a65e42cbe2c86275b0624c4971dc02d53" + \
                 "a5b77598ccedcc436af8"
        tx_hash = self.client.eth.account.send_raw_transaction(raw_tx)
        self.assertEqual(tx_hash, "0xe25bc84c719130ec49f40426b5aaeb922213d22c75cf3253aa6b3cdcfca85839")
