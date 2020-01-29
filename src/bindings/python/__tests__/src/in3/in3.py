from in3py.in3.in3 import IN3
from in3py.model.in3_model import IN3Config
from in3py.model.in3_model import RPCRequest, Address, ChainsDefinitions
from in3py.enums.in3 import EnumIn3Methods
from in3py.enums.in3 import EnumsEthCall

from in3py.runner.in3_runner import In3Runner
from in3py.utils.utils import rpc_to_string

from unittest import TestCase

import json

class In3Test(TestCase):

    def setUp(self):
        self.in3 = IN3()

    def test_config(self):
        config = IN3Config()
        config.chainId = ChainsDefinitions.KOVAN.value
        result = self.in3.config(in3_config=config)
        self.assertTrue(result.result)

    def test_address(self):
        address = Address("0x1fe2e9bf29aa1938859af64c413361227d04059a")
        checked = self.in3.checksum_address(address=address, chain=False)
        self.assertEqual("0x1Fe2E9bf29aa1938859Af64C413361227d04059a", checked.result)
        print(checked)

    def test_abi_encode(self):
        encoded = self.in3.abi_encode("getBalance(address)",["0x1234567890123456789012345678901234567890"])
        self.assertEqual("0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890",encoded.result)

    def test_abi_decode(self):
        decoded = self.in3.abi_decode("(address,uint256)", "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005")
        self.assertEqual(decoded.result[0],"0x1234567890123456789012345678901234567890")

