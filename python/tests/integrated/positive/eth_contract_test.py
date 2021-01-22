"""
Integrated tests for `in3.eth.contract` module.
"""
import unittest

import in3
from tests.integrated.mock.config import mainchain_mock_config, goerli_mock_config
from tests.integrated.mock.transport import mock_transport


class MainNetContractTest(unittest.TestCase):

    def setUp(self):
        # self.client = in3.Client(in3_config=mainchain_mock_config)
        self.client = in3.Client(
            in3_config=mainchain_mock_config, cache_enabled=False, transport=mock_transport)

    def test_eth_call(self):
        tx = {
            "to": '0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e',
            "data": '0x02571be34a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e'
        }
        transaction = in3.eth.NewTransaction(**tx)
        address = self.client.eth.contract.call(transaction)
        self.assertEqual(
            address, '0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308')

    def test_get_storage_at(self):
        storage = self.client.eth.contract.storage_at(
            "0x3589d05a1ec4Af9f65b0E5554e645707775Ee43C", 1)
        self.assertEqual(
            storage, '0x000000000000000000000000000000000000000000000000000000647261676f')

    def test_get_code(self):
        code = self.client.eth.contract.code(
            "0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e")
        self.assertEqual(len(code), 10694)

    def test_abi_encode(self):
        params = "(address,string)", "0x1234567890123456789012345678901234567890", "xyz"
        encoded = self.client.eth.contract.encode(*params)
        expected = "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000378797a0000000000000000000000000000000000000000000000000000000000"
        self.assertEqual(encoded, expected)
        params = "getData(address,string,uint8,string)", "0x1234567890123456789012345678901234567890", \
                 "xyz", "0xff", "abc"
        expected = "0x597574130000000000000000000000001234567890123456789012345678901234567890000000000000000000000" + \
                   "00000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000" + \
                   "0000000000ff00000000000000000000000000000000000000000000000000000000000000c00000000000000000000" + \
                   "00000000000000000000000000000000000000000000378797a00000000000000000000000000000000000000000000" + \
                   "00000000000000000000000000000000000000000000000000000000000000000000000000000361626300000000000" + \
                   "00000000000000000000000000000000000000000000000"
        encoded = self.client.eth.contract.encode(*params)
        self.assertEqual(encoded, expected)
        err0 = "address,string,uint8,string", "0x1234567890123456789012345678901234567890", "xyz", "0xff", "abc"
        err1 = "()", "0x1234567890123456789012345678901234567890"
        err2 = "(test)", "0x1234567890123456789012345678901234567890"
        with self.assertRaises(AssertionError):
            self.client.eth.contract.encode(*err0)
        with self.assertRaises(AssertionError):
            self.client.eth.contract.encode(*err1)
        with self.assertRaises(AssertionError):
            self.client.eth.contract.encode(*err2)

    def test_abi_decode(self):
        err0 = "address,string,uint8,string", "0x0000000000000000000000001234567890123456789012345678901234567"
        err1 = "()", "0x0000000000000000000000001234567890123456789012345678901234567"
        err2 = "(test)", "0x0000000000000000000000001234567890123456789012345678901234567"
        err3 = "(address,uint256)", "0000000000000000000000001234567890123456789012345678901234567"
        with self.assertRaises(AssertionError):
            self.client.eth.contract.decode(*err0)
        with self.assertRaises(AssertionError):
            self.client.eth.contract.decode(*err1)
        with self.assertRaises(AssertionError):
            self.client.eth.contract.decode(*err2)
        with self.assertRaises(AssertionError):
            self.client.eth.contract.decode(*err3)
        params = "(address,uint256)", "0x0000000000000000000000001234567890123456789012345678901234567890000000000" + \
                 "0000000000000000000000000000000000000000000000000000005"
        expected = ['0x1234567890123456789012345678901234567890', '0x05']
        decoded = self.client.eth.contract.decode(*params)
        self.assertEqual(decoded, expected)
        params = "(address,string,uint8,string)", \
                 "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000" + \
                 "00000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000" + \
                 "ff00000000000000000000000000000000000000000000000000000000000000c000000000000000000000000000000" + \
                 "0000000000000000000000000000000000378797a000000000000000000000000000000000000000000000000000000" + \
                 "00000000000000000000000000000000000000000000000000000000000000000003616263000000000000000000000" + \
                 "0000000000000000000000000000000000000"
        expected = ["0x1234567890123456789012345678901234567890",
                    "xyz", "0xff", "abc"]
        decoded = self.client.eth.contract.decode(*params)
        self.assertEqual(decoded, expected)


class GoerliContractTest(MainNetContractTest):

    def setUp(self):
        # self.client = in3.Client('goerli', in3_config=mock_config)
        self.client = in3.Client('goerli', in3_config=goerli_mock_config, cache_enabled=False, transport=mock_transport,
                                 test_instance=True)

    def test_eth_call(self):
        tx = {
            "to": '0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e',
            "data": '0x02571be34a17491df266270a8801cee362535e520a5d95896a719e4a7d869fb22a93162e'
        }
        transaction = in3.eth.NewTransaction(**tx)
        address = self.client.eth.contract.call(transaction)
        self.assertEqual(
            address, '0x0000000000000000000000000b56ae81586d2728ceaf7c00a6020c5d63f02308')

    def test_get_storage_at(self):
        storage = self.client.eth.contract.storage_at(
            "0x4B1488B7a6B320d2D721406204aBc3eeAa9AD329", 1)
        self.assertEqual(storage, '0x0')

    def test_get_code(self):
        code = self.client.eth.contract.code(
            "0x00000000000c2e074ec69a0dfb2997ba6c7d2e1e")
        self.assertEqual(len(code), 10694)
