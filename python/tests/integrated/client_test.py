import unittest
import in3
from tests.transport import mock_transport
from tests.config_mock import mock_config


class MainNetClientTest(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client(in3_config=mock_config)
        # self.client = in3.Client(in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client()
        self.assertIsInstance(client, in3.Client)
        config = client.get_config()
        client = in3.Client(in3_config=in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_abi_encode(self):
        params = "(address,string)", "0x1234567890123456789012345678901234567890", "xyz"
        encoded = self.client.abi_encode(*params)
        expected = "0xdd06c847000000000000000000000000123456789012345678901234567890123456789000000000000000000000" + \
                   "0000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000" + \
                   "0000000000000378797a0000000000000000000000000000000000000000000000000000000000"
        self.assertEqual(encoded, expected)
        params = "(address,string)", "1234567890123456789012345678901234567890", "xyz"
        encoded = self.client.abi_encode(*params)
        expected = "0xdd06c847000000000000000000000000000000000000000000000000313233343536373839303132333435363738" + \
                   "3930313233343536373839303132333435363738393000000000000000000000000000000000000000000000000000" + \
                   "0000000000000378797a0000000000000000000000000000000000000000000000000000000000"
        self.assertEqual(encoded, expected)
        params = "getData(address,string,uint8,string)", "0x1234567890123456789012345678901234567890", \
                 "xyz", "0xff", "abc"
        expected = "0x597574130000000000000000000000001234567890123456789012345678901234567890000000000000000000000" + \
                   "00000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000" + \
                   "0000000000ff00000000000000000000000000000000000000000000000000000000000000c00000000000000000000" + \
                   "00000000000000000000000000000000000000000000378797a00000000000000000000000000000000000000000000" + \
                   "00000000000000000000000000000000000000000000000000000000000000000000000000000361626300000000000" + \
                   "00000000000000000000000000000000000000000000000"
        encoded = self.client.abi_encode(*params)
        self.assertEqual(encoded, expected)
        err0 = "address,string,uint8,string", "0x1234567890123456789012345678901234567890", "xyz", "0xff", "abc"
        err1 = "()", "0x1234567890123456789012345678901234567890"
        err2 = "(test)", "0x1234567890123456789012345678901234567890"
        with self.assertRaises(AssertionError):
            self.client.abi_encode(*err0)
        with self.assertRaises(AssertionError):
            self.client.abi_encode(*err1)
        with self.assertRaises(AssertionError):
            self.client.abi_encode(*err2)

    def test_abi_decode(self):
        err0 = "address,string,uint8,string", "0x0000000000000000000000001234567890123456789012345678901234567"
        err1 = "()", "0x0000000000000000000000001234567890123456789012345678901234567"
        err2 = "(test)", "0x0000000000000000000000001234567890123456789012345678901234567"
        err3 = "(address,uint256)", "0000000000000000000000001234567890123456789012345678901234567"
        with self.assertRaises(AssertionError):
            self.client.abi_decode(*err0)
        with self.assertRaises(AssertionError):
            self.client.abi_decode(*err1)
        with self.assertRaises(AssertionError):
            self.client.abi_decode(*err2)
        with self.assertRaises(AssertionError):
            self.client.abi_decode(*err3)
        params = "(address,uint256)", "0x0000000000000000000000001234567890123456789012345678901234567890000000000" + \
                 "0000000000000000000000000000000000000000000000000000005"
        expected = ['0x1234567890123456789012345678901234567890', '0x05']
        decoded = self.client.abi_decode(*params)
        self.assertEqual(decoded, expected)
        params = "(address,string,uint8,string)", \
                 "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000" + \
                 "00000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000" + \
                 "ff00000000000000000000000000000000000000000000000000000000000000c000000000000000000000000000000" + \
                 "0000000000000000000000000000000000378797a000000000000000000000000000000000000000000000000000000" + \
                 "00000000000000000000000000000000000000000000000000000000000000000003616263000000000000000000000" + \
                 "0000000000000000000000000000000000000"
        expected = ["0x1234567890123456789012345678901234567890", "xyz", "0xff", "abc"]
        decoded = self.client.abi_decode(*params)
        self.assertEqual(decoded, expected)

    def test_node_list(self):
        node_list = self.client.get_node_list()
        self.assertIsInstance(node_list, in3.model.NodeList)

    def test_ens_resolve(self):
        address = self.client.ens_resolve('depraz.eth', 'addr', '0x226159d592E2b063810a10Ebf6dcbADA94Ed68b8')
        self.assertIsInstance(address, in3.model.NodeList)


class KovanClientTest(MainNetClientTest):

    def setUp(self):
        # self.client = in3.Client('kovan', in3_config=mock_client_config)
        self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client('kovan')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('kovan', in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)


class GoerliClientTest(MainNetClientTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mock_client_config)
        self.client = in3.Client('goerli', in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client('goerli')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('goerli', in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)


if __name__ == '__main__':
    unittest.main()
