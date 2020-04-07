import unittest
import in3


class UtilsTestCase(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client('goerli')

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

    def test_sign(self):
        # TODO: Check sign mock data
        result = self.client.eth.account.sign('', '')
        self.assertEqual(result, "asd")

    def test_send_tx(self):
        # TODO: Check send_tx mock data
        tx = {}
        result = self.client.eth.account.send_transaction(tx)
        self.assertEqual(result, "asd")

    def test_get_tx_receipt(self):
        tx_hash = '0x738e8878228901ad8143cfcf908abb2d2044c83231a398731841be41970a79ce'
        result = self.client.eth.account.get_transaction_receipt(tx_hash)
        self.assertIsInstance(result, in3.eth.TransactionReceipt)
