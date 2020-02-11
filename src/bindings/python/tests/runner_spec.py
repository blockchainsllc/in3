from unittest import TestCase
import in3


class In3RunnerTest(TestCase):

    @staticmethod
    def test_instance_hello_world():
        rpc = in3.RPCRequest()
        rpc.method = in3.enum.EthCall.BLOCK_NUMBER
        block_number = in3.In3Runtime.call_in3_rpc(rpc)
        print(block_number)
