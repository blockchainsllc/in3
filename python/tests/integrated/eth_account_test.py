import unittest
import in3


class UtilsTestCase(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client()

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
        self.assertEqual(address_true, '0x1fe2e9Bf29aA1938859AF64C413361227d04059a')
        # Check lower
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower()), address_true)
        self.assertEqual(self.client.eth.account.checksum_address(mixed_case_address.lower(), False), address_false)
        # Check immutable
        self.assertEqual(address_true, self.client.eth.account.checksum_address(address_true))
        self.assertEqual(address_false, self.client.eth.account.checksum_address(address_true, False))

    def test_sign(self):
        # TODO: Check sign mock data
        result = self.client.eth.account.sign('0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad', "test 123")
        self.assertEqual(result, "asd")
