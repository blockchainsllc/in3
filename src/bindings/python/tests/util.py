import unittest
import json
import in3


# TODO: Fix tests
class UtilsTestCase(unittest.TestCase):

    def test_json_rpc(self):
        filter_obj = in3.eth.Filter(fromBlock=in3.eth.Number("0x01"))
        address = in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE")
        rpc = in3.RPCRequest(method=in3.enum.EthCall.WEB3_SHA3, params=(address, filter_obj))

        print(rpc)
        # print(rpc_to_string(rpc))

    def test_from_bytes(self):
        aux = '{"jsonrpc": "2.0", "method": "web3_sha3", "params": ["0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", {"fromBlock": "0x1"}], "id": 1}'.encode(
            "utf-8")
        j = json.loads(aux.decode("utf-8"))
        f = in3.eth.Filter()
        f.__dict__.update(j["params"][1])

        print(f.fromBlock)

    # def test_get_chain_by_id(self):
    #     self.assertIs( get_chain_by_id("0x2a"), EnumsChains.KOVAN )

    def test_utf8(self):
        in3_cfg = {
            "chainId": "0x05"
        }
        rpc = in3.RPCRequest(method=in3.enum.In3Methods.CONFIG, params=(in3_cfg,))

        print(rpc.to_utf8())

    def test_validate_str_size(self):
        addresses = ["0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", "0xA87BFff94092281a435c243e6e10F9a7Fc594d26",
                     "0x07359296b25eA66E10850273CE69Cbc7cfe5Df81"]
        for a in addresses:
            in3.eth.Address(a)

    def test_hash(self):
        in3.eth.Hash("0xe8222e785eb3279b75bb8eb56457de5fb4def32f440b71bc5cc72b999dee4ab4")

    def camelCaseNameOf(self, name):
        aux = name.split("_")
        result = "";
        for a in aux:
            if a is "":
                continue
            result += a[0].upper() + a[1:]
        return result[0].lower() + result[1:]

    def test_json_to_obj(self):
        json_obj = {"from": "0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", "gas": "90000"}
        transaction = in3.eth.Transaction.from_json(data=json_obj)
        print(transaction)

    def test_json_transaction(self):
        t = in3.eth.Transaction(_from=in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
        print(t)

        # t = Transaction.from_json(json=json)
        # print(t)

        # t = Transaction(_from=Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
        #
        # json = {"from": "0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", "gas": "90000"}
        # transaction_dict = {}
        # for d in t.__dict__:
        #     aux = self.camelCaseNameOf(d)
        #     try:
        #         transaction_dict[d] = type(getattr(t,d))(json[aux])
        #     except:
        #         pass
        #
        # t.__dict__.update(transaction_dict)
        #
        # print(t._from)
        # {"from": "0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", "gas": "90000"}
        #
        #
        # transaction = Transaction(_from=Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
        # print(transaction)
