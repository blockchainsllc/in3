### Creating a new instance by "string" chainId

from in3py.client.in3_client import In3Client
from in3py.client.in3_client import IN3Config

kovan = IN3Config()
kovan.chainId= "0x2a"
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number