"""
Integrated tests for `in3` module. Doesnt test submodules.
"""
import unittest

import in3
from tests.config_mock import mock_config
from tests.transport import mock_transport


class MainNetClientTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mock_config)
        self.client = in3.Client(in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client()
        self.assertIsInstance(client, in3.Client)
        client = in3.Client(in3_config=in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_node_list(self):
        node_list = self.client.get_node_list()
        self.assertIsInstance(node_list, in3.model.NodeList)

    def test_ens_resolve(self):
        # Other calls like `addr` require more than one eth_call, being more complex to mock the tests. Suffice for now.
        address = self.client.ens_resolve('depraz.eth', 'owner')
        self.assertEqual(address, '0x0b56ae81586d2728ceaf7c00a6020c5d63f02308')


class KovanClientTest(MainNetClientTest):

    def setUp(self):
        # self.client = in3.Client('kovan', in3_config=mock_config)
        self.client = in3.Client('kovan', in3_config=mock_config, transport=mock_transport)

    def test_configure(self):
        client = in3.Client('kovan')
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
        client = in3.Client('goerli', in3.model.ClientConfig())
        self.assertIsInstance(client, in3.Client)

    def test_ens_resolve(self):
        # Other calls like `addr` require more than one eth_call, being more complex to mock the tests. Suffice for now.
        address = self.client.ens_resolve('depraz.eth', 'owner', '0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e')
        self.assertEqual(address, '0x0b56ae81586d2728ceaf7c00a6020c5d63f02308')


if __name__ == '__main__':
    unittest.main()
