/// this is generated file don't edit it manually!

import Foundation

/// a Collection of utility-function.
/// 
public class Utils {
    internal var in3: In3

    /// initialiazes the Utils API
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
    /// let result = try UtilsAPI(in3).abiEncode(signature: "getBalance(address)", params: ["0x1234567890123456789012345678901234567890"])
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
    /// let result = try UtilsAPI(in3).abiDecode(signature: "(address,uint256)", data: "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005")
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
    /// let result = try UtilsAPI(in3).checksumAddress(address: "0x1fe2e9bf29aa1938859af64c413361227d04059a", useChainId: false)
    /// // result = "0x1Fe2E9bf29aa1938859Af64C413361227d04059a"
    /// ```
    /// 
    public func checksumAddress(address: String, useChainId: Bool? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_checksumAddress", params:RPCObject( address), useChainId == nil ? RPCObject.none : RPCObject( useChainId! ), convertWith: toString )
    }

    /// converts the given value into wei.
    /// - Parameter value : the value, which may be floating number as string
    /// - Parameter unit : the unit of the value, which must be one of `wei`, `kwei`,  `Kwei`,  `babbage`,  `femtoether`,  `mwei`,  `Mwei`,  `lovelace`,  `picoether`,  `gwei`,  `Gwei`,  `shannon`,  `nanoether`,  `nano`,  `szabo`,  `microether`,  `micro`,  `finney`,  `milliether`,  `milli`,  `ether`,  `eth`,  `kether`,  `grand`,  `mether`,  `gether` or  `tether`
    /// - Returns: the value in wei as hex.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try UtilsAPI(in3).toWei(value: "20.0009123", unit: "eth")
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
    /// let result = try UtilsAPI(in3).fromWei(value: "0x234324abadefdef", unit: "eth", digits: 3)
    /// // result = "0.158"
    /// ```
    /// 
    public func fromWei(value: UInt256, unit: String, digits: Int? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_fromWei", params:RPCObject( value), RPCObject( unit), digits == nil ? RPCObject.none : RPCObject( String(format: "0x%1x", digits!) ), convertWith: toString )
    }

    /// clears the incubed cache (usually found in the .in3-folder)
    /// - Returns: true indicating the success
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try UtilsAPI(in3).cacheClear()
    /// // result = true
    /// ```
    /// 
    public func cacheClear() throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_cacheClear", convertWith: toString )
    }

    /// Returns the underlying client version. See [web3_clientversion](https://eth.wiki/json-rpc/API#web3_clientversion) for spec.
    /// - Returns: when connected to the incubed-network, `Incubed/<Version>` will be returned, but in case of a direct enpoint, its's version will be used.
    public func clientVersion() -> Future<String> {
        return execAndConvert(in3: in3, method: "web3_clientVersion", convertWith: toString )
    }

    /// Returns Keccak-256 (not the standardized SHA3-256) of the given data.
    /// 
    /// See [web3_sha3](https://eth.wiki/json-rpc/API#web3_sha3) for spec.
    /// 
    /// No proof needed, since the client will execute this locally. 
    /// 
    /// - Parameter data : data to hash
    /// - Returns: the 32byte hash of the data
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try UtilsAPI(in3).keccak(data: "0x1234567890")
    /// // result = "0x3a56b02b60d4990074262f496ac34733f870e1b7815719b46ce155beac5e1a41"
    /// ```
    /// 
    public func keccak(data: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "keccak", params:RPCObject( data), convertWith: toString )
    }

    /// Returns Keccak-256 (not the standardized SHA3-256) of the given data.
    /// 
    /// See [web3_sha3](https://eth.wiki/json-rpc/API#web3_sha3) for spec.
    /// 
    /// No proof needed, since the client will execute this locally. 
    /// 
    /// - Parameter data : data to hash
    /// - Returns: the 32byte hash of the data
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try UtilsAPI(in3).sha3(data: "0x1234567890")
    /// // result = "0x3a56b02b60d4990074262f496ac34733f870e1b7815719b46ce155beac5e1a41"
    /// ```
    /// 
    public func sha3(data: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "web3_sha3", params:RPCObject( data), convertWith: toString )
    }

    /// Returns sha-256 of the given data.
    /// 
    /// No proof needed, since the client will execute this locally. 
    /// 
    /// - Parameter data : data to hash
    /// - Returns: the 32byte hash of the data
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try UtilsAPI(in3).sha256(data: "0x1234567890")
    /// // result = "0x6c450e037e79b76f231a71a22ff40403f7d9b74b15e014e52fe1156d3666c3e6"
    /// ```
    /// 
    public func sha256(data: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "sha256", params:RPCObject( data), convertWith: toString )
    }

    /// the Network Version (currently 1)
    /// - Returns: the Version number
    public func version() throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "net_version", convertWith: toString )
    }


}
