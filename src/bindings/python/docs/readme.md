
##### Creating a new instance by chain definition

```python
import in3

kovan = IN3Config()
kovan.chainId = ChainsDefinitions.KOVAN.value
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number
```

##### Creating a new instance by "string" chainId

```python

from in3py.client.in3_client import In3Client
from in3py.client.in3_client import IN3Config

kovan = IN3Config()
kovan.chainId= "0x2a"
in3_client = In3Client(in3_config=kovan)

in3_client.eth.block_number() # Kovan's block's number

```

### Types

IN3 has some types to help you to handle all data.

##### IN3Number
```python
from in3py.model.in3_model import IN3Number

number_str = IN3Number("0x2a")
number_int = IN3Number(42)

number_int.to_hex() == number_str.to_hex() # true
number_int.to_int() == number_str.to_int() # true

print(number_str) # 0x2a
```

##### Address

In3 Address type with validations (size and checksum)

```python
from in3py.model.in3_model import Address

address = Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE", skip_validation=True)
print(address) # 0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE

# without the last 'E'
address = Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAE")
# raise AddressFormatException: The address don't have enough size as needed.

# with the last 'E' but in lowercase
address = Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEe")
# raise AddressFormatException: The Address it's not in a correct checksum.

```

##### Hash

In3 Hash type with validation (size)

```python
from in3py.model.in3_model import Hash
hash =  Hash("0x939a89debdd112ea48dc15cf491383cdfb16a5415cecef2cb396f58d8dd8d760")
print(hash) # 0x939a89debdd112ea48dc15cf491383cdfb16a5415cecef2cb396f58d8dd8d760

Hash("0x939a89debdd112ea48dc15cf49")
# raise:  HashFormatException: Hash size is not a proper size of 32 bytes

``` 

##### EnumBlockStatus

It's status of block number, you can choose between "latest", "earliest" or "pending". 

```python
from in3py.enums.in3 import EnumsBlockStatus
EnumsBlockStatus.EARLIEST
EnumsBlockStatus.LATEST
EnumsBlockStatus.PENDING

```

### Modules

Currently we have two modules, 

* In3
* Ethereum 


Other modules will be added soon.

##### Organization

All modules are inside a client instance.

```python
from in3py.client.in3_client import In3Client

in3_client = In3Client()



#### Ethereum module


You can call ethereum functions using our client

* Assuming that you have the In3 Client setted correctly as we did before, we can call these functions.



The eth module is responsible to call all ethereum functions using our client to make the things easier.




#### [gas_price](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_gasprice)

Returns the current price per gas in wei.


```python
from in3py.client.in3_client import In3Client

in3_client= In3Client()
in3_client.eth.gas_price()


```

#### [block_number](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_blockNumber)

Returns the number of most recent block.


```python
from in3py.client.in3_client import In3Client

in3_client= In3Client()
block_number  = in3_client.eth.block_number()
block_number.to_int()
block_number.to_hex()

```

#### [get_balance](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getBalance)

Returns the balance of the account of given address.

```python
address = Address("0x9E52Ee6Acd6E7F55e860844d487556f8Cbe2BAEE")
result = in3_client.eth.get_balance(address=address)
print(result.to_int())
```

#### [get_storage_at](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getStorageAt)
Returns the value from a storage position at a given address.

```python
in3_client.eth.get_storage_at(  address = Address("0xaF91fF3c7E46D40684703F514783FA8880FF8C57", skip_validation=True), 
                                position = IN3Number(0), 
                                number = EnumsBlockStatus.LATEST)

```
