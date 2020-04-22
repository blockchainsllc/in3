import unittest
import in3


class EthereumTest(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client()

    def test_ethereum_sha3(self):
        digest = self.client.eth.keccak256('0x68656c6c6f20776f726c64')
        self.assertEqual(digest, '0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad')

    def test_eth_gasPrice(self):
        self.assertGreater(self.client.eth.gas_price(), 1000000)

    def test_block_number(self):
        result = self.client.eth.block_number()
        self.assertGreater(result, 9825822)

    def test_get_balance(self):
        result = self.client.eth.get_balance("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertGreaterEqual(result, 0)
        block_number = self.client.eth.block_number() - 15
        result = self.client.eth.get_balance("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertGreaterEqual(result, 0)
        # result = self.client.eth.get_balance("0x7076D6e69315e843fB5496504F4f65127F08e2D4", 'earliest')
        # self.assertGreaterEqual(result, 0)

    def test_get_storage_at(self):
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0)
        self.assertIsInstance(storage, str)
        block_number = self.client.eth.block_number() - 15
        storage = self.client.eth.get_storage_at("0xdAC17F958D2ee523a2206206994597C13D831ec7", 0, block_number)
        self.assertIsInstance(storage, str)

    def test_get_transaction_count(self):
        rpc = self.client.eth.get_transaction_count("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertGreater(rpc, 0)
        block_number = self.client.eth.block_number() - 15
        rpc = self.client.eth.get_transaction_count("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertGreater(rpc, 0)

    def test_get_code(self):
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4")
        self.assertIsInstance(code, str)
        block_number = self.client.eth.block_number() - 15
        code = self.client.eth.get_code("0x7076D6e69315e843fB5496504F4f65127F08e2D4", block_number)
        self.assertIsInstance(code, str)

    def test_eth_call(self):
        # TODO
        transaction = in3.eth.NewTransaction(From="0x132D2A325b8d588cFB9C1188daDdD4d00193E028",
                                             to="0x7ceabea4AA352b10fBCa48e6E8015bC73687ABD4",
                                             data="0xa9c70686",
                                             nonce=5)
        rpc = self.client.eth.eth_call(transaction)
        self.assertIsInstance(rpc, str)

    def test_get_block_by_number(self):
        block_number = self.client.eth.block_number() - 15
        block = self.client.eth.get_block_by_number(block_number)
        self.assertIsInstance(block, in3.eth.Block)
        self.assertEqual(block_number, block.number)

    def test_get_transaction_by_hash(self):
        block_number = self.client.eth.block_number() - 15
        block = self.client.eth.get_block_by_number(block_number)
        tx_hash = block.transactions[0]
        tx = self.client.eth.get_transaction_by_hash(tx_hash)
        self.assertIsInstance(tx, in3.eth.Transaction)
        self.assertEqual(tx_hash, tx.hash)

# ++++++++++++++++++++++++++++++++++++++++++++
#     def test_get_transaction_by_block_number_and_index(self):
#         block_number = 15385949
#         index = 0
#         result = self.client.eth.get_transaction_by_block_number_and_index(number=block_number, index=index)
#         self.assertIsNotNone(result.blockHash)
    #    # def test_get_block_by_hash(self):
    #     hash_obj = in3.eth.Hash("0x85cf1fedb76d721bddb4197735e77b31d40e8b4f2e4e30b44fa993141b0a85d6")
    #     rpc = self.client.eth.get_block_by_hash(hash_obj=hash_obj, get_full_block=False)
    #
    #     self.assertIsNotNone(rpc.transactions)
    #     isEql = in3.eth.Hash("0x4d162e12901d1a0a0236793cc578db3b16117b276740e777c0e449478ce1239c").__eq__(rpc.transactions[0])
    #     self.assertTrue(isEql)
    #     self.assertEqual(rpc.number, 16089542)

    # def test_get_transaction_by_block_hash_and_index(self):
    #     hash_obj = in3.eth.Hash("0xc45d36c9ef34bdb174c065463aeb9775f729db63577add69e5f629917d77e646")
    #     index = 0
    #     result = self.client.eth.get_transaction_by_block_hash_and_index(block_hash=hash_obj, index=index)
    #     self.assertIsNotNone(result.blockHash)
    #
    # def test_get_block_by_hash_and_return_all_transactions_data(self):
    #     hash_obj = in3.eth.Hash("0x85cf1fedb76d721bddb4197735e77b31d40e8b4f2e4e30b44fa993141b0a85d6")
    #     rpc = self.client.eth.get_block_by_hash(hash_obj=hash_obj, get_full_block=True)
    #
    #     self.assertIsNotNone(rpc.transactions)
    #     self.assertEqual(rpc.number, 16089542)
    #     self.assertIsNotNone(rpc.transactions[0].From)
    # removed
    # in3.model.exception.In3RequestException: {'code': -32600, 'message': 'method eth_pendingTransactions is not supported or unknown'}
    # def test_pending_transactions(self):
    #     result = self.client.eth.pending_transactions()
    #     print(result)

    # removed
    # in3.model.exception.In3RequestException: {'code': -4, 'message': '[0x1]:The Method cannot be verified with eth_nano!'}
    # def test_get_uncle_by_block_hash_and_index(self):
    #     #  check later for the  uncle by block
    #     hash_obj = in3.eth.Hash("0xa2163d7d18578e0995b1304003b857337eaa4534cbe64905c7bd45a744932f1f")
    #     index = 0
    #     result = self.client.eth.get_uncle_by_block_hash_and_index(block_hash=hash_obj, index=index)
    #     print(result)

    # removed
    # in3.model.exception.In3RequestException: {'code': -4, 'message': '[0x1]:The Method cannot be verified with eth_nano!'}
    # def test_get_uncle_by_block_number_and_index(self):
    #     #  check later for the  uncle by block
    #     block_number = 5066978
    #     index = 0
    #     result = self.client.eth.get_uncle_by_block_number_and_index(number=block_number, index=index)
    #     print(result)

    # def test_new_filter(self):
    #     filter_obj = in3.eth.Filter(fromBlock=16095289)
    #     response = self.client.eth.new_filter(filter=filter_obj)
    #     self.assertEqual(response, "0x1")
    #     import time
    #
    #     time.sleep(1)
    #     changes = self.client.eth.get_filter_logs(filter_id=response)
    #     print(changes)
    #
    #
    # def test_new_block_filter(self):
    #     filter_obj = self.client.eth.new_block_filter()
    #     import time
    #     time.sleep(10)
    #     changes = self.client.eth.get_filter_changes(filter_id=filter_obj)
    #     for c in changes:
    #         block = self.client.eth.get_block_by_hash(c,False)
    #         self.assertIsNotNone(block.hash)
    #     self.assertTrue(len(changes)>0)
    #     self.assertEqual(filter_obj, "0x1")
    #
    # # removed
    # # {'code': -3, 'message': 'The request could not be handled pending filter not supported'}
    # # def test_new_pending_transaction_filter(self):
    # #     filter_obj = self.client.eth.new_pending_transaction_filter()
    # #     print(filter_obj)
    #
    # def test_uninstall_filter(self):
    #     filter_obj = in3.eth.Filter(fromBlock=in3.BlockStatus.LATEST, toBlock=in3.BlockStatus.EARLIEST,
    #                                 address=in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
    #     response = self.client.eth.new_filter(filter=filter_obj)
    #     filter_id = int(response, 16)
    #     result = self.client.eth.uninstall_filter(filter_id=filter_id)
    #     self.assertTrue(result)
    #
    # # in3.model.exception.In3RequestException: {'code': -9, 'message': 'The request could not be handled failed to get filter changes internal error, call to eth_getLogs failed E'}
    # def test_get_filter_changes(self):
    #     filter_obj = in3.eth.Filter(fromBlock=in3.BlockStatus.LATEST, toBlock=in3.BlockStatus.EARLIEST,
    #                                 address=in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
    #     response = self.client.eth.new_filter(filter=filter_obj)
    #     filter_id = int(response, 16)
    #     filter = self.client.eth.get_filter_changes(filter_id=filter_id)
    #     print(filter)
    #
    # # removed
    # # in3.model.exception.In3RequestException: {'code': -32600, 'message': 'method eth_getFilterLogs is not supported or unknown'}
    # # def test_get_filter_logs(self):
    # #     filter_obj = in3.eth.Filter(fromBlock=in3.BlockStatus.LATEST, toBlock=in3.BlockStatus.EARLIEST,
    # #                                 address=in3.eth.Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE"))
    # #     response = self.client.eth.new_filter(filter=filter_obj)
    # #     filter_id = int(response, 16)
    # #     result = self.client.eth.get_filter_logs(filter_id=filter_id)
    # #     print(result)
    #
    # # in3.model.exception.In3RequestException: {'code': -32600, 'message': 'eth_getLogs : params[0].fromBlock (a quantity hex number) should match pattern "^0x(0|[a-fA-F1-9]+[a-fA-F0-9]*)$"'}
    # def test_get_logs(self):
    #     filter_obj = in3.eth.Filter(fromBlock=16095389, toBlock=in3.BlockStatus.LATEST)
    #     logs = self.client.eth.get_logs(from_filter=filter_obj)
    #     self.assertTrue(logs)
    #
    #
    # def test_prepare_transaction(self):
    #
    #     aux = {
    #         'to': '0xA87BFff94092281a435c243e6e10F9a7Fc594d26',
    #         'value': 1000000,
    #         'gas': 2000000,
    #         'nonce': 78,
    #         'gasPrice': 5000000000,
    #         'chainId': hex(42)
    #     }
    #
    #     to = Address("0xA87BFff94092281a435c243e6e10F9a7Fc594d26")
    #     transaction = Transaction(to=to, value=1000000, gasPrice=5000000000, gas=2000000, nonce=78)
    #
    #     transaction_str = in3.client.prepare_transaction(transaction)
    #     print(transaction_str)
    #
    #
    # def test_signature(self):
    #     size = "0xf868"
    #     transaction_signature ="4e85012a05f200831e848094a87bfff94092281a435c243e6e10f9a7fc594d26830f424080"
    #     appended = "77a00e812f21c773ffd8e39da74371dfd5d1f0b626af1fd4d493880e6face4acd094a025ad6e719ba2b51b3407ef6dc71e2f9dbe81ab37986c04d270c7330b715027d1"
    #
    #     in3.client.send_transaction(size + transaction_signature + appended)


if __name__ == '__main__':
    unittest.main()
