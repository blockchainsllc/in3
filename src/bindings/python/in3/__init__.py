"""
  ,,
  db

`7MM  `7MMpMMMb.  pd""b.
  MM    MM    MM (O)  `8b
  MM    MM    MM      ,89
  MM    MM    MM    ""Yb.
.JMML..JMML  JMML.     88
                 (O)  .M'
                  bmmmd'

Incubed - Your favorite web3 provider for connecting embedded devices to the blockchain
"""
from in3.model.exception import IN3BaseException, In3AddressFormatException, In3HashFormatException, \
    In3NumberFormatException, In3PrivateKeyNotFoundException, In3RequestException
from in3.model.client import Config, ChainSpec, RPCRequest, RPCResponse
from in3.model.enum import Chain, BlockStatus
from in3.bind.runtime import In3Runtime
from in3.client import Client

import in3.eth as eth

__name__ = 'in3'
__author__ = 'Slock.it <github.com/slockit>'
__repository__ = 'github.com/slockit/in3-c'
__status__ = "pre-alpha"
__version__ = "2.1"
__date__ = "10 December 2019"    # Merry X-Mas!
