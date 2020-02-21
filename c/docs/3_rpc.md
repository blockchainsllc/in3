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


* **autoUpdateList** :`bool` *(optional)*  - if true the nodelist will be automaticly updated if the lastBlock is newer.
    example: true

* **chainId** :`uint32_t` or `string (mainnet/kovan/goerli)` - servers to filter for the given chain. The chain-id based on EIP-155.
    example: 0x1

* **signatureCount** :`uint8_t` *(optional)*  - number of signatures requested.
    example: 2
    
* **finality** :`uint16_t` *(optional)*  - the number in percent needed in order reach finality (% of signature of the validators).
    example: 50

* **includeCode** :`bool` *(optional)*  - if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards.
    example: true

* **maxAttempts** :`uint16_t` *(optional)*  - max number of attempts in case a response is rejected.
    example: 10

* **keepIn3** :`bool` *(optional)*  - if true, requests sent to the input sream of the comandline util will be send theor responses in the same form as the server did.
    example: false

* **key** :`bytes32` *(optional)*  - the client key to sign requests.
    example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7

* **useBinary** :`bool` *(optional)*  - if true the client will use binary format.
    example: false

* **useHttp** :`bool` *(optional)*  - if true the client will try to use http instead of https.
    example: false

* **maxBlockCache** :`uint32_t` *(optional)*  - number of number of blocks cached  in memory.
    example: 100

* **maxCodeCache** :`uint32_t` *(optional)*  - number of max bytes used to cache the code in memory.
    example: 100000

* **timeout** :`uint32_t` *(optional)*  - specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection.
    example: 100000

* **minDeposit** :`uint64_t` - min stake of the server. Only nodes owning at least this amount will be chosen.

* **nodeProps** :`uint64_t` bitmask *(optional)*  - used to identify the capabilities of the node.

* **nodeLimit** :`uint16_t` *(optional)*  - the limit of nodes to store in the client.
    example: 150

* **proof** :`string (none/standard/full)` *(optional)*  - if true the nodes should send a proof of the response.
    example: true

* **replaceLatestBlock** :`uint8_t` *(optional)*  - if specified, the blocknumber *latest* will be replaced by blockNumber- specified value.
    example: 6

* **requestCount** :`uint8_t` - the number of request send when getting a first answer.
    example: 3

* **rpc** :`string` *(optional)*  - url of one or more rpc-endpoints to use. (list can be comma seperated)

* **servers**/**nodes** : `collection of JSON objects with chain Id (hex string) as key` *(optional)*  - the value of each JSON object defines the nodelist per chain and may contain the following fields:
    
    * **contract** :`address`  - address of the registry contract.
    * **whiteListContract** :`address` *(optional, cannot be combined with whiteList)*  - address of the whiteList contract.
    * **whiteList** :`array of addresses` *(optional, cannot be combined with whiteListContract)*  - manual whitelist.
    * **registryId** :`bytes32`  - identifier of the registry.
    * **needsUpdate** :`bool` *(optional)*  - if set, the nodeList will be updated before next request.
    * **avgBlockTime** :`uint16_t` *(optional)*  - average block time (seconds) for this chain.
    * **verifiedHashes** :`array of JSON objects` *(optional)*  - if the client sends an array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request. MUST contain the following fields:

        * **block** :`uint64_t`  - block number.
        * **hash** : `bytes32`  - verified hash corresponding to block number.

    * **nodeList** :`array of JSON objects` *(optional)*  - manual nodeList, each JSON object may contain the following fields:
    
        * **url** :`string`  - URL of the node.
        * **address** :`address`  - address of the node.
        * **props** :`uint64_t` bitmask *(optional)*  - used to identify the capabilities of the node (defaults to 65535).

Returns:

an boolean confirming that the config has changed.

Example:


Request:
```js
{
	"method": "in3_config",
	"params": [{
		"chainId": "0x5",
		"maxAttempts": 4,
		"nodeLimit": 10,
		"servers": {
			"0x1": {
				"nodeList": [{
						"address": "0x1234567890123456789012345678901234567890",
						"url": "https://mybootnode-A.com",
						"props": "0xFFFF"
					},
					{
						"address": "0x1234567890123456789012345678901234567890",
						"url": "https://mybootnode-B.com",
						"props": "0xFFFF"
					}
				]
			}
		}
	}]
}
```

Response:

```js
{
  "id": 1,
  "result": true
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


### in3_ens

resolves a ens-name.
the domain names consist of a series of dot-separated labels. Each label must be a valid normalised label as described in [UTS46](https://unicode.org/reports/tr46/) with the options `transitional=false` and `useSTD3AsciiRules=true`. For Javascript implementations, a [library](https://www.npmjs.com/package/idna-uts46) is available that normalises and checks names.

Parameters:

1. `name`: string - the domain name UTS46 compliant string.
2. `field`: string - the required data, which could be
    - `addr` - the address ( default )
    - `resolver` - the address of the resolver
    - `hash` - the namehash 
    - `owner` - the owner of the domain




Returns:

the address-string using the upper/lowercase hex characters.

Request:

```js
{
    "method":"in3_ens",
    "params":[
        "cryptokitties.eth",
        "addr"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0x06012c8cf97bead5deae237070f9587f8e7a266d"
}
```




### in3_pk2address

extracts the address from a private key.

Parameters:

1. `key`: hex - the 32 bytes private key as hex.

Returns:

the address-string.

Request:

```js
{
    "method":"in3_pk2address",
    "params":[
        "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0xdc5c4280d8a286f0f9c8f7f55a5a0c67125efcfd"
}
```


### in3_pk2public

extracts the public key from a private key.

Parameters:

1. `key`: hex - the 32 bytes private key as hex.

Returns:

the public key.

Request:

```js
{
    "method":"in3_pk2public",
    "params":[
        "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0x0903329708d9380aca47b02f3955800179e18bffbb29be3a644593c5f87e4c7fa960983f78186577eccc909cec71cb5763acd92ef4c74e5fa3c43f3a172c6de1"
}
```



### in3_ecrecover

extracts the public key and address from signature.

Parameters:

1. `msg`: hex - the message the signature is based on.
2. `sig`: hex - the 65 bytes signature as hex.
3. `sigtype`: string - the type of the signature data : `eth_sign` (use the prefix and hash it), `raw` (hash the raw data), `hash` (use the already hashed data). Default: `raw`

Returns:

a object with 2 properties:

- `publicKey` : hex - the 64 byte public key
- `address` : address - the 20 byte address

Request:

```js
{
    "method":"in3_ecrecover",
    "params":[
        "0x487b2cbb7997e45b4e9771d14c336b47c87dc2424b11590e32b3a8b9ab327999",
        "0x0f804ff891e97e8a1c35a2ebafc5e7f129a630a70787fb86ad5aec0758d98c7b454dee5564310d497ddfe814839c8babd3a727692be40330b5b41e7693a445b71c",
        "hash"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": {
      "publicKey": "0x94b26bafa6406d7b636fbb4de4edd62a2654eeecda9505e9a478a66c4f42e504c4481bad171e5ba6f15a5f11c26acfc620f802c6768b603dbcbe5151355bbffb",
      "address":"0xf68a4703314e9a9cf65be688bd6d9b3b34594ab4"
   }
}
```

### in3_signData

signs the given data

Parameters:

1. `msg`: hex - the message to sign.
2. `key`: hex - the key (32 bytes) or address (20 bytes) of the signer. If the address is passed, the internal signer needs to support this address.
3. `sigtype`: string - the type of the signature data : `eth_sign` (use the prefix and hash it), `raw` (hash the raw data), `hash` (use the already hashed data). Default: `raw`

Returns:

a object with the following properties:

- `message` : hex - original message used
- `messageHash` : hex - the hash the signature is based on
- `signature`: hex - the signature (65 bytes)
- `r` : hex - the x -value of the EC-Point
- `s` : hex - the y -value of the EC-Point
- `v` : number - the sector (0|1) + 27

Request:

```js
{
    "method":"in3_signData",
    "params":[
        "0x0102030405060708090a0b0c0d0e0f",
        "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852",
        "raw"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": {
      "message":"0x0102030405060708090a0b0c0d0e0f",
      "messageHash":"0x1d4f6fccf1e27711667605e29b6f15adfda262e5aedfc5db904feea2baa75e67",
      "signature":"0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e95792264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e1521b",
      "r":"0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e9579",
      "s":"0x2264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e152",
      "v":27
   }
}
```


### in3_decryptKey

decrypts a JSON Keystore file as defined in the [Web3 Secret Storage Definition
](https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition). The result is the raw private key.

Parameters:

1. `key`: Object - Keydata as object as defined in the keystorefile
2. `passphrase`: String - the password to decrypt it.

Returns:

a raw private key (32 bytes)


Request:

```js
{
    "method": "in3_decryptKey",
    "params": [
        {
            "version": 3,
            "id": "f6b5c0b1-ba7a-4b67-9086-a01ea54ec638",
            "address": "08aa30739030f362a8dd597fd3fcde283e36f4a1",
            "crypto": {
                "ciphertext": "d5c5aafdee81d25bb5ac4048c8c6954dd50c595ee918f120f5a2066951ef992d",
                "cipherparams": {
                    "iv": "415440d2b1d6811d5c8a3f4c92c73f49"
                },
                "cipher": "aes-128-ctr",
                "kdf": "pbkdf2",
                "kdfparams": {
                    "dklen": 32,
                    "salt": "691e9ad0da2b44404f65e0a60cf6aabe3e92d2c23b7410fd187eeeb2c1de4a0d",
                    "c": 16384,
                    "prf": "hmac-sha256"
                },
                "mac": "de651c04fc67fd552002b4235fa23ab2178d3a500caa7070b554168e73359610"
            }
        },
        "test"
    ]
}
```

Response:

```js
{
  "id": 1,
  "result": "0x1ff25594a5e12c1e31ebd8112bdf107d217c1393da8dc7fc9d57696263457546"
}
```



### in3_cacheClear

clears the incubed cache (usually found in the .in3-folder)


Request:

```js
{
    "method":"in3_cacheClear",
    "params":[]
}
```

Response:

```js
{
  "id": 1,
  "result": true
}
```
