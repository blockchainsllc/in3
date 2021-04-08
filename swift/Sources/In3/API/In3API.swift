/// this is generated file don't edit it manually!

import Foundation

/// There are also some Incubed specific rpc-methods, which will help the clients to bootstrap and update the nodeLists.
/// 
/// 
/// The incubed client itself offers special RPC-Methods, which are mostly handled directly inside the client:
/// 
public class In3API {
    internal var in3: In3

    /// initialiazes the In3 API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function encodes the value given and returns it as hexstring.
    /// - Parameter signature : the signature of the function. e.g. `getBalance(uint256)`. The format is the same as used by solidity to create the functionhash. optional you can also add the return type, which in this case is ignored.
    /// - Parameter params : a array of arguments. the number of arguments must match the arguments in the signature.
    /// - Returns: the ABI-encoded data as hex including the 4 byte function-signature. These data can be used for `eth_call` or to send a transaction.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).abiEncode(signature: "getBalance(address)", params: ["0x1234567890123456789012345678901234567890"])
    /// // result = "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890"
    /// ```
    /// 
    public func abiEncode(signature: String, params: [AnyObject]) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_abiEncode", params:RPCObject( signature), RPCObject( params), convertWith: toString )
    }

    /// based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function decodes the bytes given and returns it as array of values.
    /// - Parameter signature : the signature of the function. e.g. `uint256`, `(address,string,uint256)` or `getBalance(address):uint256`. If the complete functionhash is given, only the return-part will be used.
    /// - Parameter data : the data to decode (usually the result of a eth_call)
    /// - Returns: a array with the values after decodeing.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).abiDecode(signature: "(address,uint256)", data: "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005")
    /// // result = 
    /// //          - "0x1234567890123456789012345678901234567890"
    /// //          - "0x05"
    /// ```
    /// 
    public func abiDecode(signature: String, data: String) throws ->  [RPCObject] {
        return try execLocalAndConvert(in3: in3, method: "in3_abiDecode", params:RPCObject( signature), RPCObject( data), convertWith: { try toArray($0,$1)! } )
    }

    /// Will convert an upper or lowercase Ethereum address to a checksum address.  (See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md) )
    /// - Parameter address : the address to convert.
    /// - Parameter useChainId : if true, the chainId is integrated as well (See [EIP1191](https://github.com/ethereum/EIPs/issues/1121) )
    /// - Returns: the address-string using the upper/lowercase hex characters.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).checksumAddress(address: "0x1fe2e9bf29aa1938859af64c413361227d04059a", useChainId: false)
    /// // result = "0x1Fe2E9bf29aa1938859Af64C413361227d04059a"
    /// ```
    /// 
    public func checksumAddress(address: String, useChainId: Bool? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_checksumAddress", params:RPCObject( address), useChainId == nil ? RPCObject.none : RPCObject( useChainId! ), convertWith: toString )
    }

    /// resolves a ens-name. 
    /// the domain names consist of a series of dot-separated labels. Each label must be a valid normalised label as described in [UTS46](https://unicode.org/reports/tr46/) with the options `transitional=false` and `useSTD3AsciiRules=true`. 
    /// For Javascript implementations, a [library](https://www.npmjs.com/package/idna-uts46) is available that normalises and checks names.
    /// 
    /// - Parameter name : the domain name UTS46 compliant string.
    /// - Parameter field : the required data, which could be one of ( `addr` - the address, `resolver` - the address of the resolver, `hash` - the namehash, `owner` - the owner of the domain)
    /// - Returns: the value of the specified field
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).ens(name: "cryptokitties.eth", field: "addr") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x1Fe2E9bf29aa1938859Af64C413361227d04059a"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func ens(name: String, field: String? = "addr") -> Future<String> {
        return execAndConvert(in3: in3, method: "in3_ens", params:RPCObject( name), field == nil ? RPCObject.none : RPCObject( field! ), convertWith: toString )
    }

    /// converts the given value into wei.
    /// - Parameter value : the value, which may be floating number as string
    /// - Parameter unit : the unit of the value, which must be one of `wei`, `kwei`,  `Kwei`,  `babbage`,  `femtoether`,  `mwei`,  `Mwei`,  `lovelace`,  `picoether`,  `gwei`,  `Gwei`,  `shannon`,  `nanoether`,  `nano`,  `szabo`,  `microether`,  `micro`,  `finney`,  `milliether`,  `milli`,  `ether`,  `eth`,  `kether`,  `grand`,  `mether`,  `gether` or  `tether`
    /// - Returns: the value in wei as hex.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).toWei(value: "20.0009123", unit: "eth")
    /// // result = "0x01159183c4793db800"
    /// ```
    /// 
    public func toWei(value: String, unit: String? = "eth") throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_toWei", params:RPCObject( value), unit == nil ? RPCObject.none : RPCObject( unit! ), convertWith: toString )
    }

    /// converts a given uint (also as hex) with a wei-value into a specified unit.
    /// - Parameter value : the value in wei
    /// - Parameter unit : the unit of the target value, which must be one of `wei`, `kwei`,  `Kwei`,  `babbage`,  `femtoether`,  `mwei`,  `Mwei`,  `lovelace`,  `picoether`,  `gwei`,  `Gwei`,  `shannon`,  `nanoether`,  `nano`,  `szabo`,  `microether`,  `micro`,  `finney`,  `milliether`,  `milli`,  `ether`,  `eth`,  `kether`,  `grand`,  `mether`,  `gether` or  `tether`
    /// - Parameter digits : fix number of digits after the comma. If left out, only as many as needed will be included.
    /// - Returns: the value as string.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).fromWei(value: "0x234324abadefdef", unit: "eth", digits: 3)
    /// // result = "0.158"
    /// ```
    /// 
    public func fromWei(value: UInt256, unit: String, digits: Int? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_fromWei", params:RPCObject( value), RPCObject( unit), digits == nil ? RPCObject.none : RPCObject( String(format: "0x%1x", digits!) ), convertWith: toString )
    }

    /// extracts the address from a private key.
    /// - Parameter pk : the 32 bytes private key as hex.
    /// - Returns: the address
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).pk2address(pk: "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a")
    /// // result = "0xdc5c4280d8a286f0f9c8f7f55a5a0c67125efcfd"
    /// ```
    /// 
    public func pk2address(pk: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_pk2address", params:RPCObject( pk), convertWith: toString )
    }

    /// extracts the public key from a private key.
    /// - Parameter pk : the 32 bytes private key as hex.
    /// - Returns: the public key as 64 bytes
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).pk2public(pk: "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a")
    /// // result = "0x0903329708d9380aca47b02f3955800179e18bffbb29be3a644593c5f87e4c7fa960983f7818\
    /// //          6577eccc909cec71cb5763acd92ef4c74e5fa3c43f3a172c6de1"
    /// ```
    /// 
    public func pk2public(pk: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_pk2public", params:RPCObject( pk), convertWith: toString )
    }

    /// extracts the public key and address from signature.
    /// - Parameter msg : the message the signature is based on.
    /// - Parameter sig : the 65 bytes signature as hex.
    /// - Parameter sigtype : the type of the signature data : `eth_sign` (use the prefix and hash it), `raw` (hash the raw data), `hash` (use the already hashed data). Default: `raw`
    /// - Returns: the extracted public key and address
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).ecrecover(msg: "0x487b2cbb7997e45b4e9771d14c336b47c87dc2424b11590e32b3a8b9ab327999", sig: "0x0f804ff891e97e8a1c35a2ebafc5e7f129a630a70787fb86ad5aec0758d98c7b454dee5564310d497ddfe814839c8babd3a727692be40330b5b41e7693a445b71c", sigtype: "hash")
    /// // result = 
    /// //          publicKey: "0x94b26bafa6406d7b636fbb4de4edd62a2654eeecda9505e9a478a66c4f42e504c\
    /// //            4481bad171e5ba6f15a5f11c26acfc620f802c6768b603dbcbe5151355bbffb"
    /// //          address: "0xf68a4703314e9a9cf65be688bd6d9b3b34594ab4"
    /// ```
    /// 
    public func ecrecover(msg: String, sig: String, sigtype: String? = "raw") throws ->  In3Ecrecover {
        return try execLocalAndConvert(in3: in3, method: "in3_ecrecover", params:RPCObject( msg), RPCObject( sig), sigtype == nil ? RPCObject.none : RPCObject( sigtype! ), convertWith: { try In3Ecrecover($0,$1) } )
    }

    /// prepares a Transaction by filling the unspecified values and returens the unsigned raw Transaction.
    /// - Parameter tx : the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
    /// - Returns: the unsigned raw transaction as hex.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).prepareTx(tx: In3Transaction(to: "0x63f666a23cbd135a91187499b5cc51d589c302a0", value: "0x100000000", from: "0xc2b2f4ad0d234b8c135c39eea8409b448e5e496f")) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xe980851a13b865b38252089463f666a23cbd135a91187499b5cc51d589c302a0850100000000\
    /// //          80018080"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func prepareTx(tx: In3Transaction) -> Future<String> {
        return execAndConvert(in3: in3, method: "in3_prepareTx", params:RPCObject( tx.toRPCDict()), convertWith: toString )
    }

    /// signs the given raw Tx (as prepared by in3_prepareTx ). The resulting data can be used in `eth_sendRawTransaction` to publish and broadcast the transaction.
    /// - Parameter tx : the raw unsigned transactiondata
    /// - Parameter from : the account to sign
    /// - Returns: the raw transaction with signature.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).signTx(tx: "0xe980851a13b865b38252089463f666a23cbd135a91187499b5cc51d589c302a085010000000080018080", from: "0xc2b2f4ad0d234b8c135c39eea8409b448e5e496f") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xf86980851a13b865b38252089463f666a23cbd135a91187499b5cc51d589c302a08501000000\
    /// //          008026a03c5b094078383f3da3f65773ab1314e89ee76bc41f827f2ef211b2d3449e4435a077755\
    /// //          f8d9b32966e1ad8f6c0e8c9376a4387ed237bdbf2db6e6b94016407e276"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func signTx(tx: String, from: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "in3_signTx", params:RPCObject( tx), RPCObject( from), convertWith: toString )
    }

    /// signs the given data.
    /// - Parameter msg : the message to sign.
    /// - Parameter account : the account to sign if the account is a bytes32 it will be used as private key
    /// - Parameter msgType : the type of the signature data : `eth_sign` (use the prefix and hash it), `raw` (hash the raw data), `hash` (use the already hashed data)
    /// - Returns: the signature
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).signData(msg: "0x0102030405060708090a0b0c0d0e0f", account: "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852", msgType: "raw") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          message: "0x0102030405060708090a0b0c0d0e0f"
    /// //          messageHash: "0x1d4f6fccf1e27711667605e29b6f15adfda262e5aedfc5db904feea2baa75e67"
    /// //          signature: "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e95792\
    /// //            264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e1521b"
    /// //          r: "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e9579"
    /// //          s: "0x2264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e152"
    /// //          v: 27
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func signData(msg: String, account: String, msgType: String? = "raw") -> Future<In3SignData> {
        return execAndConvert(in3: in3, method: "in3_signData", params:RPCObject( msg), RPCObject( account), msgType == nil ? RPCObject.none : RPCObject( msgType! ), convertWith: { try In3SignData($0,$1) } )
    }

    /// decrypts a JSON Keystore file as defined in the [Web3 Secret Storage Definition](https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition). The result is the raw private key.
    /// - Parameter key : Keydata as object as defined in the keystorefile
    /// - Parameter passphrase : the password to decrypt it.
    /// - Returns: a raw private key (32 bytes)
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).decryptKey(key: {"version":"3,","id":"f6b5c0b1-ba7a-4b67-9086-a01ea54ec638","address":"08aa30739030f362a8dd597fd3fcde283e36f4a1","crypto":{"ciphertext":"d5c5aafdee81d25bb5ac4048c8c6954dd50c595ee918f120f5a2066951ef992d","cipherparams":{"iv":"415440d2b1d6811d5c8a3f4c92c73f49"},"cipher":"aes-128-ctr","kdf":"pbkdf2","kdfparams":{"dklen":32,"salt":"691e9ad0da2b44404f65e0a60cf6aabe3e92d2c23b7410fd187eeeb2c1de4a0d","c":16384,"prf":"hmac-sha256"},"mac":"de651c04fc67fd552002b4235fa23ab2178d3a500caa7070b554168e73359610"}}, passphrase: "test")
    /// // result = "0x1ff25594a5e12c1e31ebd8112bdf107d217c1393da8dc7fc9d57696263457546"
    /// ```
    /// 
    public func decryptKey(key: String, passphrase: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_decryptKey", params:RPCObject( key), RPCObject( passphrase), convertWith: toString )
    }

    /// clears the incubed cache (usually found in the .in3-folder)
    /// - Returns: true indicating the success
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try In3API(in3).cacheClear()
    /// // result = true
    /// ```
    /// 
    public func cacheClear() throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_cacheClear", convertWith: toString )
    }

    /// fetches and verifies the nodeList from a node
    /// - Parameter limit : if the number is defined and >0 this method will return a partial nodeList limited to the given number.
    /// - Parameter seed : this 32byte hex integer is used to calculate the indexes of the partial nodeList. It is expected to be a random value choosen by the client in order to make the result deterministic.
    /// - Parameter addresses : a optional array of addresses of signers the nodeList must include.
    /// - Returns: the current nodelist
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).nodeList(limit: 2, seed: "0xe9c15c3b26342e3287bb069e433de48ac3fa4ddd32a31b48e426d19d761d7e9b", addresses: []) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          totalServers: 5
    /// //          contract: "0x64abe24afbba64cae47e3dc3ced0fcab95e4edd5"
    /// //          registryId: "0x423dd84f33a44f60e5d58090dcdcc1c047f57be895415822f211b8cd1fd692e3"
    /// //          lastBlockNumber: 8669495
    /// //          nodes:
    /// //            - url: https://in3-v2.slock.it/mainnet/nd-3
    /// //              address: "0x945F75c0408C0026a3CD204d36f5e47745182fd4"
    /// //              index: 2
    /// //              deposit: "10000000000000000"
    /// //              props: 29
    /// //              timeout: 3600
    /// //              registerTime: 1570109570
    /// //              weight: 2000
    /// //              proofHash: "0x27ffb9b7dc2c5f800c13731e7c1e43fb438928dd5d69aaa8159c21fb13180a4c"
    /// //            - url: https://in3-v2.slock.it/mainnet/nd-5
    /// //              address: "0xbcdF4E3e90cc7288b578329efd7bcC90655148d2"
    /// //              index: 4
    /// //              deposit: "10000000000000000"
    /// //              props: 29
    /// //              timeout: 3600
    /// //              registerTime: 1570109690
    /// //              weight: 2000
    /// //              proofHash: "0xd0dbb6f1e28a8b90761b973e678cf8ecd6b5b3a9d61fb9797d187be011ee9ec7"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func nodeList(limit: Int? = nil, seed: String? = nil, addresses: [String]? = nil) -> Future<In3NodeList> {
        return execAndConvert(in3: in3, method: "in3_nodeList", params:limit == nil ? RPCObject.none : RPCObject( String(format: "0x%1x", limit!) ), seed == nil ? RPCObject.none : RPCObject( seed! ), addresses == nil ? RPCObject.none : RPCObject( addresses! ), convertWith: { try In3NodeList($0,$1) } )
    }

    /// requests a signed blockhash from the node. 
    /// In most cases these requests will come from other nodes, because the client simply adds the addresses of the requested signers 
    /// and the processising nodes will then aquire the signatures with this method from the other nodes.
    /// 
    /// Since each node has a risk of signing a wrong blockhash and getting convicted and losing its deposit, 
    /// per default nodes will and should not sign blockHash of the last `minBlockHeight` (default: 6) blocks!
    /// 
    /// - Parameter blocks : array of requested blocks.
    /// - Returns: the Array with signatures of all the requires blocks.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).sign(blocks: In3Blocks(blockNumber: 8770580)) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          - blockHash: "0xd8189793f64567992eaadefc51834f3d787b03e9a6850b8b9b8003d8d84a76c8"
    /// //            block: 8770580
    /// //            r: "0x954ed45416e97387a55b2231bff5dd72e822e4a5d60fa43bc9f9e49402019337"
    /// //            s: "0x277163f586585092d146d0d6885095c35c02b360e4125730c52332cf6b99e596"
    /// //            v: 28
    /// //            msgHash: "0x40c23a32947f40a2560fcb633ab7fa4f3a96e33653096b17ec613fbf41f946ef"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func sign(blocks: In3Blocks) -> Future<In3Sign> {
        return execAndConvert(in3: in3, method: "in3_sign", params:RPCObject( blocks.toRPCDict()), convertWith: { try In3Sign($0,$1) } )
    }

    /// Returns whitelisted in3-nodes addresses. The whitelist addressed are accquired from whitelist contract that user can specify in request params.
    /// - Parameter address : address of whitelist contract
    /// - Returns: the whitelisted addresses
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).whitelist(address: "0x08e97ef0a92EB502a1D7574913E2a6636BeC557b") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          totalServers: 2
    /// //          contract: "0x08e97ef0a92EB502a1D7574913E2a6636BeC557b"
    /// //          lastBlockNumber: 1546354
    /// //          nodes:
    /// //            - "0x1fe2e9bf29aa1938859af64c413361227d04059a"
    /// //            - "0x45d45e6ff99e6c34a235d263965910298985fcfe"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func whitelist(address: String) -> Future<In3Whitelist> {
        return execAndConvert(in3: in3, method: "in3_whitelist", params:RPCObject( address), convertWith: { try In3Whitelist($0,$1) } )
    }

    /// adds a raw private key as signer, which allows signing transactions.
    /// - Parameter pk : the 32byte long private key as hex string.
    /// - Returns: the address of given key.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).addRawKey(pk: "0x1234567890123456789012345678901234567890123456789012345678901234") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x2e988a386a799f506693793c6a5af6b54dfaabfb"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func addRawKey(pk: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "in3_addRawKey", params:RPCObject( pk), convertWith: toString )
    }

    /// returns a array of account-addresss the incubed client is able to sign with. In order to add keys, you can use [in3_addRawKey](#in3-addrawkey) or configure them in the config. The result also contains the addresses of any signer signer-supporting the `PLGN_ACT_SIGN_ACCOUNT` action.
    /// - Returns: the array of addresses of all registered signers.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// In3API(in3).accounts() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          - "0x2e988a386a799f506693793c6a5af6b54dfaabfb"
    /// //          - "0x93793c6a5af6b54dfaabfb2e988a386a799f5066"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func accounts() -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_accounts", convertWith: toString )
    }


}
/// the extracted public key and address
public struct In3Ecrecover {
    /// the public Key of the signer (64 bytes)
    public var publicKey: String

    /// the address
    public var address: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        publicKey = try toString(obj["publicKey"],false)!
        address = try toString(obj["address"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["publicKey"] = RPCObject(publicKey)
        obj["address"] = RPCObject(address)
        return obj
    }
}

/// the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
public struct In3Transaction {
    /// receipient of the transaction.
    public var to: String

    /// sender of the address (if not sepcified, the first signer will be the sender)
    public var from: String

    /// value in wei to send
    public var value: UInt256?

    /// the gas to be send along
    public var gas: UInt64?

    /// the price in wei for one gas-unit. If not specified it will be fetched using `eth_gasPrice`
    public var gasPrice: UInt64?

    /// the current nonce of the sender. If not specified it will be fetched using `eth_getTransactionCount`
    public var nonce: UInt64?

    /// the data-section of the transaction
    public var data: String?

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        to = try toString(obj["to"],false)!
        from = try toString(obj["from"],false)!
        value = try toUInt256(obj["value"],true)!
        gas = try toUInt64(obj["gas"],true)!
        gasPrice = try toUInt64(obj["gasPrice"],true)!
        nonce = try toUInt64(obj["nonce"],true)!
        data = try toString(obj["data"],true)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["to"] = RPCObject(to)
        obj["from"] = RPCObject(from)
        obj["value"] = value == nil ? RPCObject.none : RPCObject(value!)
        obj["gas"] = gas == nil ? RPCObject.none : RPCObject(gas!)
        obj["gasPrice"] = gasPrice == nil ? RPCObject.none : RPCObject(gasPrice!)
        obj["nonce"] = nonce == nil ? RPCObject.none : RPCObject(nonce!)
        obj["data"] = data == nil ? RPCObject.none : RPCObject(data!)
        return obj
    }
}

/// the signature
public struct In3SignData {
    /// original message used
    public var message: String

    /// the hash the signature is based on
    public var messageHash: String

    /// the signature (65 bytes)
    public var signature: String

    /// the x-value of the EC-Point
    public var r: String

    /// the y-value of the EC-Point
    public var s: String

    /// the recovery value (0|1) + 27
    public var v: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        message = try toString(obj["message"],false)!
        messageHash = try toString(obj["messageHash"],false)!
        signature = try toString(obj["signature"],false)!
        r = try toString(obj["r"],false)!
        s = try toString(obj["s"],false)!
        v = try toString(obj["v"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["message"] = RPCObject(message)
        obj["messageHash"] = RPCObject(messageHash)
        obj["signature"] = RPCObject(signature)
        obj["r"] = RPCObject(r)
        obj["s"] = RPCObject(s)
        obj["v"] = RPCObject(v)
        return obj
    }
}

/// the current nodelist
public struct In3NodeList {
    /// a array of node definitions.
    public var nodes: In3Nodes

    /// the address of the Incubed-storage-contract. The client may use this information to verify that we are talking about the same contract or throw an exception otherwise.
    public var contract: String

    /// the registryId (32 bytes)  of the contract, which is there to verify the correct contract.
    public var registryId: String

    /// the blockNumber of the last change of the list (usually the last event).
    public var lastBlockNumber: UInt64

    /// the total numbers of nodes.
    public var totalServer: UInt64

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        nodes = try In3Nodes(obj["nodes"],false)!
        contract = try toString(obj["contract"],false)!
        registryId = try toString(obj["registryId"],false)!
        lastBlockNumber = try toUInt64(obj["lastBlockNumber"],false)!
        totalServer = try toUInt64(obj["totalServer"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["contract"] = RPCObject(contract)
        obj["registryId"] = RPCObject(registryId)
        obj["lastBlockNumber"] = RPCObject(lastBlockNumber)
        obj["totalServer"] = RPCObject(totalServer)
        return obj
    }
}

/// a array of node definitions.
public struct In3Nodes {
    /// the url of the node. Currently only http/https is supported, but in the future this may even support onion-routing or any other protocols.
    public var url: String

    /// the address of the signer
    public var address: String

    /// the index within the nodeList of the contract
    public var index: UInt64

    /// the stored deposit
    public var deposit: UInt256

    /// the bitset of capabilities as described in the [Node Structure](spec.html#node-structure)
    public var props: String

    /// the time in seconds describing how long the deposit would be locked when trying to unregister a node.
    public var timeout: UInt64

    /// unix timestamp in seconds when the node has registered.
    public var registerTime: UInt64

    /// the weight of a node ( not used yet ) describing the amount of request-points it can handle per second.
    public var weight: UInt64

    /// a hash value containing the above values. 
    /// This hash is explicitly stored in the contract, which enables the client to have only one merkle proof 
    /// per node instead of verifying each property as its own storage value. 
    /// The proof hash is build `keccak256( abi.encodePacked( deposit, timeout, registerTime, props, signer, url ))` 
    /// 
    public var proofHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        url = try toString(obj["url"],false)!
        address = try toString(obj["address"],false)!
        index = try toUInt64(obj["index"],false)!
        deposit = try toUInt256(obj["deposit"],false)!
        props = try toString(obj["props"],false)!
        timeout = try toUInt64(obj["timeout"],false)!
        registerTime = try toUInt64(obj["registerTime"],false)!
        weight = try toUInt64(obj["weight"],false)!
        proofHash = try toString(obj["proofHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["url"] = RPCObject(url)
        obj["address"] = RPCObject(address)
        obj["index"] = RPCObject(index)
        obj["deposit"] = RPCObject(deposit)
        obj["props"] = RPCObject(props)
        obj["timeout"] = RPCObject(timeout)
        obj["registerTime"] = RPCObject(registerTime)
        obj["weight"] = RPCObject(weight)
        obj["proofHash"] = RPCObject(proofHash)
        return obj
    }
}

/// array of requested blocks.
public struct In3Blocks {
    /// the blockNumber to sign
    public var blockNumber: UInt64

    /// the expected hash. This is optional and can be used to check if the expected hash is correct, but as a client you should not rely on it, but only on the hash in the signature.
    public var hash: String?

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        hash = try toString(obj["hash"],true)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["blockNumber"] = RPCObject(blockNumber)
        obj["hash"] = hash == nil ? RPCObject.none : RPCObject(hash!)
        return obj
    }
}

/// the Array with signatures of all the requires blocks.
public struct In3Sign {
    /// the blockhash which was signed.
    public var blockHash: String

    /// the blocknumber
    public var block: UInt64

    /// r-value of the signature
    public var r: String

    /// s-value of the signature
    public var s: String

    /// v-value of the signature
    public var v: String

    /// the msgHash signed. This Hash is created with `keccak256( abi.encodePacked( _blockhash,  _blockNumber, registryId ))`
    public var msgHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        blockHash = try toString(obj["blockHash"],false)!
        block = try toUInt64(obj["block"],false)!
        r = try toString(obj["r"],false)!
        s = try toString(obj["s"],false)!
        v = try toString(obj["v"],false)!
        msgHash = try toString(obj["msgHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["blockHash"] = RPCObject(blockHash)
        obj["block"] = RPCObject(block)
        obj["r"] = RPCObject(r)
        obj["s"] = RPCObject(s)
        obj["v"] = RPCObject(v)
        obj["msgHash"] = RPCObject(msgHash)
        return obj
    }
}

/// the whitelisted addresses
public struct In3Whitelist {
    /// array of whitelisted nodes addresses.
    public var nodes: String

    /// the blockNumber of the last change of the in3 white list event.
    public var lastWhiteList: UInt64

    /// whitelist contract address.
    public var contract: String

    /// the blockNumber of the last change of the list (usually the last event).
    public var lastBlockNumber: UInt64

    /// the total numbers of whitelist nodes.
    public var totalServer: UInt64

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        nodes = try toString(obj["nodes"],false)!
        lastWhiteList = try toUInt64(obj["lastWhiteList"],false)!
        contract = try toString(obj["contract"],false)!
        lastBlockNumber = try toUInt64(obj["lastBlockNumber"],false)!
        totalServer = try toUInt64(obj["totalServer"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["nodes"] = RPCObject(nodes)
        obj["lastWhiteList"] = RPCObject(lastWhiteList)
        obj["contract"] = RPCObject(contract)
        obj["lastBlockNumber"] = RPCObject(lastBlockNumber)
        obj["totalServer"] = RPCObject(totalServer)
        return obj
    }
}