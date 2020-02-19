import unittest
import in3


class In3ApiTest(unittest.TestCase):

    def setUp(self):
        conf = in3.Config()
        conf.chainId = str(in3.Chain.KOVAN)
        self.in3_client = in3.Client(in3_config=conf)

    def test_in3_config(self):
        conf_kovan = in3.Config()
        conf_kovan.chainId = str(in3.Chain.KOVAN)
        self.in3_client.in3.config(in3_config=conf_kovan)
        block_kovan = self.in3_client.eth.block_number()

        conf_main = in3.Config()
        conf_main.chainId = str(in3.Chain.MAINNET)
        self.in3_client.in3.config(in3_config=conf_main)
        block_main = self.in3_client.eth.block_number()

        self.assertNotEqual(block_kovan,block_main)


    def test_abi_encode(self):
        encoded = self.in3_client.in3.abi_encode("getBalance(address)", ["0x1234567890123456789012345678901234567890"])
        self.assertEqual("0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890", encoded)

    def test_abi_decode(self):
        decoded = self.in3_client.in3.abi_decode("(address,uint256)", "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005")
        add0, add1 = decoded
        self.assertEqual(add0, "0x1234567890123456789012345678901234567890")
        self.assertEqual(add1, "0x05")

    def test_checksum_address(self):
        address = "0x1Eb661a24d39a83B333c7E92975E92e3C3b2D028"
        checked = self.in3_client.in3.checksum_address(address=address.upper(), chain=False)
        self.assertEqual(checked,address )

if __name__ == '__main__':
    unittest.main()
