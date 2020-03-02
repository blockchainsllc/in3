import unittest
import in3


# TODO: Add negative tests
class In3NumberTest(unittest.TestCase):

    def test_instance_from_hex(self):
        kovan_number = in3.eth.Number(0x2a)
        self.assertTrue(hex(int(kovan_number)) == "0x2a")

    def test_instance_from_string(self):
        number_str = in3.Number("0x2a")
        self.assertTrue(hex(int(number_str)) == "0x2a")

    def test_instance_from_int(self):
        number_int = in3.Number(42)
        self.assertTrue(int(number_int) == 42)


class AddressTest(unittest.TestCase):

    def test_address_instance(self):
        address = in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", skip_validation=False)
        self.assertTrue("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE" == str(address))


class HashTest(unittest.TestCase):

    def test_hash_negative(self):
        with self.assertRaises(in3.In3HashFormatException):
            in3.eth.Hash("0x939a89debdd112ea48dc15cf491383cdfb16a5415cecef2cb396f58d8dd8d76", skip_validation=False)

    def test_hash(self):
        hash_obj = in3.eth.Hash("0x6505610ab6d21ac7200dafc20f135e1d19fbe201e5daa754c24bec6b97d57bb9")
        self.assertTrue("0x6505610ab6d21ac7200dafc20f135e1d19fbe201e5daa754c24bec6b97d57bb9" == str(hash_obj))
