# RPC

The core of incubed is to execute rpc-requests which will be send to the incubed nodes and verified. This means the available RPC-Requests are defined by the clients itself.

- For Ethereum : https://github.com/ethereum/wiki/wiki/JSON-RPC
- For Bitcoin : https://bitcoincore.org/en/doc/0.18.0/

The Incbed nodes already add a few special RPC-methods, which are specified in the [RPC-Specification](https://in3.readthedocs.io/en/develop/spec.html#incubed) Section of the Protocol.

In addition the incubed client itself offers special RPC-Methods, which are mostly handled directly inside the client:

### in3_config

changes the configuration of a client. The configuration is passed as the first param and may contain only the values to change.

Parameters:

1. `config`: config-object - a Object with config-params.

The config params support the following properties :


* **[autoUpdateList](https://github.com/slockit/in3/blob/master/src/types/types.ts#L255)** :`boolean` *(optional)*  - if true the nodelist will be automaticly updated if the lastBlock is newer
    example: true

* **[chainId](https://github.com/slockit/in3/blob/master/src/types/types.ts#L240)** :`string` - servers to filter for the given chain. The chain-id based on EIP-155.
    example: 0x1

* **[finality](https://github.com/slockit/in3/blob/master/src/types/types.ts#L230)** :`number` *(optional)*  - the number in percent needed in order reach finality (% of signature of the validators)
    example: 50

* **[includeCode](https://github.com/slockit/in3/blob/master/src/types/types.ts#L187)** :`boolean` *(optional)*  - if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards
    example: true

* **[keepIn3](https://github.com/slockit/in3/blob/master/src/types/types.ts#L187)** :`boolean` *(optional)*  - if true, requests sent to the input sream of the comandline util will be send theor responses in the same form as the server did.
    example: false

* **[key](https://github.com/slockit/in3/blob/master/src/types/types.ts#L169)** :`any` *(optional)*  - the client key to sign requests
    example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7

* **[maxAttempts](https://github.com/slockit/in3/blob/master/src/types/types.ts#L182)** :`number` *(optional)*  - max number of attempts in case a response is rejected
    example: 10

* **[maxBlockCache](https://github.com/slockit/in3/blob/master/src/types/types.ts#L197)** :`number` *(optional)*  - number of number of blocks cached  in memory
    example: 100

* **[maxCodeCache](https://github.com/slockit/in3/blob/master/src/types/types.ts#L192)** :`number` *(optional)*  - number of max bytes used to cache the code in memory
    example: 100000

* **[minDeposit](https://github.com/slockit/in3/blob/master/src/types/types.ts#L215)** :`number` - min stake of the server. Only nodes owning at least this amount will be chosen.

* **[nodeLimit](https://github.com/slockit/in3/blob/master/src/types/types.ts#L155)** :`number` *(optional)*  - the limit of nodes to store in the client.
    example: 150

* **[proof](https://github.com/slockit/in3/blob/master/src/types/types.ts#L206)** :`'none'`|`'standard'`|`'full'` *(optional)*  - if true the nodes should send a proof of the response
    example: true

* **[replaceLatestBlock](https://github.com/slockit/in3/blob/master/src/types/types.ts#L220)** :`number` *(optional)*  - if specified, the blocknumber *latest* will be replaced by blockNumber- specified value
    example: 6

* **[requestCount](https://github.com/slockit/in3/blob/master/src/types/types.ts#L225)** :`number` - the number of request send when getting a first answer
    example: 3

* **[rpc](https://github.com/slockit/in3/blob/master/src/types/types.ts#L267)** :`string` *(optional)*  - url of one or more rpc-endpoints to use. (list can be comma seperated)

* **[servers](https://github.com/slockit/in3/blob/master/src/types/types.ts#L271)** *(optional)*  - the nodelist per chain

* **[signatureCount](https://github.com/slockit/in3/blob/master/src/types/types.ts#L211)** :`number` *(optional)*  - number of signatures requested
    example: 2

* **[verifiedHashes](https://github.com/slockit/in3/blob/master/src/types/types.ts#L201)** :`string`[] *(optional)*  - if the client sends a array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.

Returns:

an boolean confirming that the config has changed.

Example:


Request:
```js
{
  "method":"in3_config",
  "params":[{
      "chainId":"0x5",
      "maxAttempts":4,
      "nodeLimit":10
      "servers":{
          "0x1": [
              "nodeList": [
                  {
                    "address":"0x1234567890123456789012345678901234567890",
                    "url":"https://mybootnode-A.com",
                    "props":"0xFFFF",
                  },
                  {
                    "address":"0x1234567890123456789012345678901234567890",
                    "url":"https://mybootnode-B.com",
                    "props":"0xFFFF",
                  }
              ]
          ]
      }

   }]
}
```

Response:

```js
{
  "id": 1,
  "result": true,
}
```

### in3_abiEncode

based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function encodes the values and returns it as hex-string.

Parameters:

1. `signature`: string - the signature of the function. e.g. `getBalance(uint256)`. The format is the same as used by solidity to create the functionhash. optional you can also add the return type, which in this case is ignored.
2. `params`: array - a array of arguments. the number of arguments must match the arguments in the signature.


Returns:

the ABI-encoded data as hex including the 4 byte function-signature. These data can be used for `eth_call` or to send a transaction.

Request:

```js
{
    "method":"in3_abiEncode",
    "params":[
        "getBalance(address)",
        ["0x1234567890123456789012345678901234567890"]
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890",
}
```


### in3_abiDecode

based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function decodes the bytes given and returns it as array of values.

Parameters:

1. `signature`: string - the signature of the function. e.g. `uint256`, `(address,string,uint256)` or `getBalance(address):uint256`. If the complete functionhash is given, only the return-part will be used.
2. `data`: hex - the data to decode (usually the result of a eth_call)

Returns:

a array (if more then one arguments in the result-type) or the the value after decodeing.

Request:

```js
{
    "method":"in3_abiDecode",
    "params":[
        "(address,uint256)",
        "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": ["0x1234567890123456789012345678901234567890","0x05"],
}
```


### in3_checksumAddress

Will convert an upper or lowercase Ethereum address to a checksum address.  (See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md) )

Parameters:

1. `address`: address - the address to convert.
2. `useChainId`: boolean - if true, the chainId is integrated as well (See [EIP1191](https://github.com/ethereum/EIPs/issues/1121) )

Returns:

the address-string using the upper/lowercase hex characters.

Request:

```js
{
    "method":"in3_checksumAddress",
    "params":[
        "0x1fe2e9bf29aa1938859af64c413361227d04059a",
        false
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0x1Fe2E9bf29aa1938859Af64C413361227d04059a"
}
```