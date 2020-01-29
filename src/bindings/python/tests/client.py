import unittest
import in3


# TODO: Negative tests
class In3ClientTest(unittest.TestCase):

    def setUp(self):
        self.client = in3.Client()

    def test_reach_kovan(self):
        kovan_cfg = in3.Config()
        kovan_cfg.chainId = in3.Chain.KOVAN.value['chain_id']

        self.client.in3.config(kovan_cfg)
        bl = self.client.eth.block_number()
        self.assertIsNotNone(bl)

    def test_reach_kovan_built_in(self):
        kovan_cfg = in3.Config()
        kovan_cfg.chainId = str(in3.Chain.KOVAN)

        self.client.in3.config(kovan_cfg)
        bl = self.client.eth.block_number()
        self.assertIsNotNone(bl)

    def test_reach_kovan_hardcoded(self):
        kovan_cfg = in3.Config()
        kovan_cfg.chainId = "0x2a"

        self.client.in3.config(kovan_cfg)
        bl = self.client.eth.block_number()
        self.assertIsNotNone(bl)

    def test_reach_goerli(self):
        goerli_cfg = in3.Config()
        goerli_cfg.chainId = str(in3.Chain.GOERLI)

        self.client.in3.config(goerli_cfg)
        bl = self.client.eth.block_number()
        self.assertIsNotNone(bl)

    def test_reach_main(self):
        main_net_cfg = in3.Config()
        main_net_cfg.chainId = str(in3.Chain.MAINNET)

        self.client.in3.config(main_net_cfg)
        nl = self.client.in3.node_list()
        self.assertIsNotNone(nl)
        ns = self.client.in3.node_stats()
        self.assertIsNotNone(ns)
        bl2 = self.client.eth.block_number()
        self.assertIsNotNone(bl2)


if __name__ == '__main__':
    unittest.main()
