from in3.model.client import Config

import in3.eth as eth
import in3.in3 as in3


class Client:
    """
    In3 Client and web3 provider
    """

    def __init__(self, in3_config: Config = None):
        self.in3 = in3
        self.eth = eth
        if in3_config is not None:
            self.in3.config(in3_config=in3_config)
