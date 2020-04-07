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

    def test_estimate_gas(self):
        transaction = in3.eth.RawTransaction(
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

    # def test_sign(self):
    #     # TODO: Check sign mock data
    #     result = self.client.eth.account.sign('', '')
    #     self.assertEqual(result, "asd")
    #
    # def test_send_tx(self):
    #     # TODO: Check send_tx mock data
    #     tx = {}
    #     result = self.client.eth.account.send_transaction(tx)
    #     self.assertEqual(result, "asd")
    #
    #
    # def test_send_raw_transaction(self):
    #     # TODO: test_send_raw_transaction
    #     # it will fail if we didn't update the nonce
    #     data = "0xf8674184ee6b2800831e848094a87bfff94092281a435c243e6e10f9a7fc594d26830f42408078a06f7d65ea8a4d69c41f8c824afb2b2e6d2bc2622d9d906b0ea8d45b39f4853931a062ad406c693c629d8a4a1af29d21e672d11c577a5a220267562bb10612c8eab7"
    #     rpc = self.client.eth.send_raw_transaction(data=data)
    #     print(rpc)