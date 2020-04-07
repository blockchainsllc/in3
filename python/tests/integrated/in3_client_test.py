import unittest
import in3


class In3ClientTest(unittest.TestCase):

    def setUp(self):
        self.in3client = in3.Client()

    def test_configure(self):
        client = in3.Client()
        self.assertIsNotNone(client)
        client = in3.Client(in3.model.ClientConfig())
        self.assertIsNotNone(client)
        # client = in3.Client(in3.ClientConfig(**in3.model.MAINNET.__dict__))
        # self.assertIsNotNone(client)
        client = in3.Client('mainnet')
        self.assertIsNotNone(client)

    def test_node_list(self):
        nl = self.in3client.node_list()
        self.assertIsInstance(nl, in3.model.NodeList)

    # def test_abi_encode(self):
    #     # TODO: Check abi encode mock data
    #     param1 = ""
    #     param2 = 123
    #     encoded_evm_rpc = self.in3client.abi_encode("", param1, param2)
    #     self.assertEqual(encoded_evm_rpc, "")
    #
    # def test_abi_decode(self):
    #     # TODO: Check abi decode mock data
    #     param1, param2 = self.in3client.abi_decode("", "")
    #     self.assertEqual(param1, "")
    #     self.assertEqual(param2, 123)
    #
    # def test_abi_encode2(self):
    #     encoded = self.client.eth.account.abi_encode("getBalance(address)", ["0x1234567890123456789012345678901234567890"])
    #     self.assertEqual("0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890", encoded)
    #
    # def test_abi_decode2(self):
    #     decoded = self.client.eth.account.abi_decode("(address,uint256)", "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005")
    #     add0, add1 = decoded
    #     self.assertEqual(add0, "0x1234567890123456789012345678901234567890")
    #     self.assertEqual(add1, "0x05")


class In3ClientKovanTest(In3ClientTest):

    def setUp(self):
        self.in3client = in3.Client('kovan')

    def test_configure(self):
        client = in3.Client('kovan')
        self.assertIsNotNone(client)
        # client = in3.Client(str(in3.model.Chain.KOVAN))
        # self.assertIsNotNone(client)


class In3ClientGoerliTest(In3ClientTest):

    def setUp(self):
        self.in3client = in3.Client('goerli')

    def test_configure(self):
        client = in3.Client('goerli')
        self.assertIsNotNone(client)
        # client = in3.Client(str(in3.model.Chain.GOERLI))
        # self.assertIsNotNone(client)


if __name__ == '__main__':
    unittest.main()
