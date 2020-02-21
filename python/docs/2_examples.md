## Examples

### blocknumber

source : [in3-c/python/examples/blocknumber.py](https://github.com/slockit/in3-c/blob/master/python/examples/blocknumber.py)



```python
### Creating a new instance by chain definition

import in3

kovan = IN3Config()
kovan.chainId = ChainsDefinitions.KOVAN.value
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number
```

### chainId

source : [in3-c/python/examples/chainId.py](https://github.com/slockit/in3-c/blob/master/python/examples/chainId.py)



```python
### Creating a new instance by "string" chainId

from in3py.client.in3_client import In3Client
from in3py.client.in3_client import IN3Config

kovan = IN3Config()
kovan.chainId= "0x2a"
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number
```


### Building 

In order to run those examples, you need to install in3 first

```sh
pip install in3
```

In order to run a example use

```
python blocknumber.py
```

