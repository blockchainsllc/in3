"""
Integrated tests for `in3` module. Doesnt test submodules.
"""
import unittest

import in3
from tests.integrated.mock.config import mock_config
from tests.integrated.mock.transport import mock_transport


class MainNetClientTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mock_config)
        self.client = in3.Client(in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client()
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('mainnet')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('mainNet')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client(in3_config=in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_node_list(self):
        node_list = self.client.refresh_node_list()
        self.assertIsInstance(node_list, in3.model.NodeList)

    def test_ens_resolve(self):
        # Other calls like `addr` require more than one eth_call, being more complex to mock the tests. Suffice for now.
        address = self.client.ens_owner('depraz.eth')
        self.assertEqual(address, '0x0b56ae81586d2728ceaf7c00a6020c5d63f02308')

    def test_ens_namehash(self):
        foo_name = self.client.ens_namehash('foo.eth')
        self.assertEqual(foo_name, '0xde9b09fd7c5f901e23a3f19fecc54828e9c848539801e86591bd9801b019f84f')
        self.assertIsNotNone(self.client.ens_namehash('123.eth'))
        self.assertIsNotNone(self.client.ens_namehash('x.eth'))
        self.assertIsNotNone(self.client.ens_namehash('zxc.eth'))
        self.assertIsNotNone(self.client.ens_namehash('0.eth'))


class KovanClientTest(MainNetClientTest):

    def setUp(self):
        # self.client = in3.Client('kovan', in3_config=mock_config)
        self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client('kovan')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('koVan')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('kovan', in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_ens_resolve(self):
        # Not supported by app.ens.domains!
        return


class GoerliClientTest(MainNetClientTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mock_config)
        self.client = in3.Client('goerli', in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client('goerli')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('goErli')
        self.assertIsInstance(client, in3.Client)
        client = in3.Client('goerli', in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_ens_resolve(self):
        # Other calls like `addr` require more than one eth_call, being more complex to mock the tests. Suffice for now.
        address = self.client.ens_owner('depraz.eth')
        self.assertEqual(address, '0x0b56ae81586d2728ceaf7c00a6020c5d63f02308')


if __name__ == '__main__':
    unittest.main()
