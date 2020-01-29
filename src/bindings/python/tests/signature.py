
import unittest
import in3
from in3.model.client import Address, Transaction


class SignatureTest(unittest.TestCase):

    def setUp(self):
        conf = in3.Config()
        conf.chainId = str(in3.Chain.KOVAN)
        self.in3_client = in3.Client(in3_config=conf)

    def test_prepare_transaction(self):

        aux = {
            'to': '0xA87BFff94092281a435c243e6e10F9a7Fc594d26',
            'value': 1000000,
            'gas': 2000000,
            'nonce': 78,
            'gasPrice': 5000000000,
            'chainId': hex(42)
        }

        to = Address("0xA87BFff94092281a435c243e6e10F9a7Fc594d26")
        transaction = Transaction(to=to, value=1000000, gasPrice=5000000000, gas=2000000, nonce=78)

        transaction_str = self.in3_client.in3.prepare_transaction(transaction)
        print(transaction_str)


    def test_signature(self):
        size = "0xf868"
        transaction_signature ="4e85012a05f200831e848094a87bfff94092281a435c243e6e10f9a7fc594d26830f424080"
        appended = "77a00e812f21c773ffd8e39da74371dfd5d1f0b626af1fd4d493880e6face4acd094a025ad6e719ba2b51b3407ef6dc71e2f9dbe81ab37986c04d270c7330b715027d1"

        self.in3_client.in3.send_transaction(size+transaction_signature+appended)







if __name__ == '__main__':
    unittest.main()
