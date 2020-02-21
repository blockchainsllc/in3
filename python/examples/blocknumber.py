### Creating a new instance by chain definition

import in3

kovan = IN3Config()
kovan.chainId = ChainsDefinitions.KOVAN.value
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number