"""
Integrated negative tests for `in3` module. Doesnt test submodules.
"""
import unittest

import in3
from tests.integrated.mock.config import mainchain_mock_config
from tests.integrated.mock.transport import mock_transport


class ClientNegativeTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_configure(self):
        # TODO
        pass

    def test_ens_owner(self):
        for i in range(50):
            with self.assertRaises(in3.ClientException):
                self.client.ens_owner('depraz.eth', '')
            with self.assertRaises(in3.ClientException):
                self.client.ens_owner('deprazz.eth')

    def test_ens_resolver(self):
        for i in range(50):
            with self.assertRaises(in3.ClientException):
                self.client.ens_resolver('depraz.eth', '')
            with self.assertRaises(in3.ClientException):
                self.client.ens_owner('deprazz.eth')

    def test_ens_address(self):
        for i in range(50):
            with self.assertRaises(in3.ClientException):
                self.client.ens_address('depraz.eth', '')
            with self.assertRaises(in3.ClientException):
                self.client.ens_owner('deprazz.eth')

    def test_ens_namehash(self):
        for i in range(50):
            with self.assertRaises(in3.ClientException):
                self.client.ens_namehash('0x0.eth')


class ClientParsingTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_instantiate(self):
        with self.assertRaises(AssertionError):
            in3.Client(None)
        with self.assertRaises(AssertionError):
            in3.Client(1)
        with self.assertRaises(AssertionError):
            in3.Client(-1)
        with self.assertRaises(AssertionError):
            in3.Client('œ∑´´†√¨')
        with self.assertRaises(AssertionError):
            in3.Client('!@# asd')
        with self.assertRaises(AssertionError):
            in3.Client({1: 1})
        with self.assertRaises(AssertionError):
            in3.Client((1))
        with self.assertRaises(AssertionError):
            in3.Client([1])

    def test_configure(self):
        with self.assertRaises(AssertionError):
            in3.Client(in3_config=1)
        with self.assertRaises(AssertionError):
            in3.Client(in3_config=-1)
        with self.assertRaises(AssertionError):
            in3.Client(in3_config='œ∑´´†√¨')
        with self.assertRaises(AssertionError):
            in3.Client(in3_config={1: 1})
        with self.assertRaises(AssertionError):
            in3.Client(in3_config=(1))
        with self.assertRaises(AssertionError):
            in3.Client(in3_config=[1])

    def test_ens_owner(self):
        # Other calls like `addr` require more than one eth_call, being more complex to mock the tests. Suffice for now.
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_owner('depraz.eth', '0x')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_owner('depraz.eth', '¡™£¢∞§¶•')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_owner('depraz.eth', 132)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_owner('depraz.eth', True)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_owner('depraz.eth', '123')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_owner('depraz')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_owner('depraz.')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_owner('depraz.etho')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_owner('.eth')

    def test_ens_resolver(self):
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_resolver('depraz.eth', '0x')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_resolver('depraz.eth', '¡™£¢∞§¶•')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_resolver('depraz.eth', 132)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_resolver('depraz.eth', True)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_resolver('depraz.eth', '123')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_resolver('depraz')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_resolver('depraz.')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_resolver('depraz.etho')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_resolver('.eth')

    def test_ens_address(self):
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_address('depraz.eth', '0x')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_address('depraz.eth', '¡™£¢∞§¶•')
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_address('depraz.eth', 132)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_address('depraz.eth', True)
        with self.assertRaises(in3.EthAddressFormatException):
            self.client.ens_address('depraz.eth', '123')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_address('depraz')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_address('depraz.')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_address('depraz.etho')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_address('.eth')

    def test_ens_namehash(self):
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_namehash('depraz')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_namehash('depraz.')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_namehash('depraz.etho')
        with self.assertRaises(in3.exception.EnsDomainFormatException):
            self.client.ens_namehash('.eth')


class GoerliClientTest(ClientNegativeTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mainchain_mock_config)
        self.client = in3.Client('goerli', in3_config=mainchain_mock_config, cache_enabled=False,
                                 transport=mock_transport, test_instance=True)


if __name__ == '__main__':
    unittest.main()
