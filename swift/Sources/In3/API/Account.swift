/// this is generated file don't edit it manually!

import Foundation

/// Account Handling includes handling signers and preparing and signing transacrtion and data.
/// 
/// Signers are Plugins able to create signatures. Those functions will use the registered plugins.
/// 
public class Account {
    internal var in3: In3

    /// initialiazes the Account API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// extracts the address from a private key.
    /// - Parameter pk : the 32 bytes private key as hex.
    /// - Returns: the address
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try AccountAPI(in3).pk2address(pk: "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a")
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
    /// let result = try AccountAPI(in3).pk2public(pk: "0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a")
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
    /// let result = try AccountAPI(in3).ecrecover(msg: "0x487b2cbb7997e45b4e9771d14c336b47c87dc2424b11590e32b3a8b9ab327999", sig: "0x0f804ff891e97e8a1c35a2ebafc5e7f129a630a70787fb86ad5aec0758d98c7b454dee5564310d497ddfe814839c8babd3a727692be40330b5b41e7693a445b71c", sigtype: "hash")
    /// // result = 
    /// //          publicKey: "0x94b26bafa6406d7b636fbb4de4edd62a2654eeecda9505e9a478a66c4f42e504c\
    /// //            4481bad171e5ba6f15a5f11c26acfc620f802c6768b603dbcbe5151355bbffb"
    /// //          address: "0xf68a4703314e9a9cf65be688bd6d9b3b34594ab4"
    /// ```
    /// 
    public func ecrecover(msg: String, sig: String, sigtype: String? = "raw") throws ->  AccountEcrecover {
        return try execLocalAndConvert(in3: in3, method: "in3_ecrecover", params:RPCObject( msg), RPCObject( sig), sigtype == nil ? RPCObject.none : RPCObject( sigtype! ), convertWith: { try AccountEcrecover($0,$1) } )
    }

    /// prepares a Transaction by filling the unspecified values and returens the unsigned raw Transaction.
    /// - Parameter tx : the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
    /// - Returns: the unsigned raw transaction as hex.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// AccountAPI(in3).prepareTx(tx: AccountTransaction(to: "0x63f666a23cbd135a91187499b5cc51d589c302a0", value: "0x100000000", from: "0xc2b2f4ad0d234b8c135c39eea8409b448e5e496f")) .observe(using: {
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
    public func prepareTx(tx: AccountTransaction) -> Future<String> {
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
    /// AccountAPI(in3).signTx(tx: "0xe980851a13b865b38252089463f666a23cbd135a91187499b5cc51d589c302a085010000000080018080", from: "0xc2b2f4ad0d234b8c135c39eea8409b448e5e496f") .observe(using: {
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
    /// AccountAPI(in3).signData(msg: "0x0102030405060708090a0b0c0d0e0f", account: "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852", msgType: "raw") .observe(using: {
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
    public func signData(msg: String, account: String, msgType: String? = "raw") -> Future<AccountSignData> {
        return execAndConvert(in3: in3, method: "in3_signData", params:RPCObject( msg), RPCObject( account), msgType == nil ? RPCObject.none : RPCObject( msgType! ), convertWith: { try AccountSignData($0,$1) } )
    }

    /// decrypts a JSON Keystore file as defined in the [Web3 Secret Storage Definition](https://github.com/ethereum/wiki/wiki/Web3-Secret-Storage-Definition). The result is the raw private key.
    /// - Parameter key : Keydata as object as defined in the keystorefile
    /// - Parameter passphrase : the password to decrypt it.
    /// - Returns: a raw private key (32 bytes)
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try AccountAPI(in3).decryptKey(key: {"version":"3,","id":"f6b5c0b1-ba7a-4b67-9086-a01ea54ec638","address":"08aa30739030f362a8dd597fd3fcde283e36f4a1","crypto":{"ciphertext":"d5c5aafdee81d25bb5ac4048c8c6954dd50c595ee918f120f5a2066951ef992d","cipherparams":{"iv":"415440d2b1d6811d5c8a3f4c92c73f49"},"cipher":"aes-128-ctr","kdf":"pbkdf2","kdfparams":{"dklen":32,"salt":"691e9ad0da2b44404f65e0a60cf6aabe3e92d2c23b7410fd187eeeb2c1de4a0d","c":16384,"prf":"hmac-sha256"},"mac":"de651c04fc67fd552002b4235fa23ab2178d3a500caa7070b554168e73359610"}}, passphrase: "test")
    /// // result = "0x1ff25594a5e12c1e31ebd8112bdf107d217c1393da8dc7fc9d57696263457546"
    /// ```
    /// 
    public func decryptKey(key: String, passphrase: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_decryptKey", params:RPCObject( key), RPCObject( passphrase), convertWith: toString )
    }

    /// Generates 32 random bytes.
    /// If /dev/urandom is available it will be used and should generate a secure random number.
    /// If not the number should not be considered sceure or used in production.
    /// 
    /// - Parameter seed : the seed. If given the result will be deterministic.
    /// - Returns: the 32byte random data
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try AccountAPI(in3).createKey()
    /// // result = "0x6c450e037e79b76f231a71a22ff40403f7d9b74b15e014e52fe1156d3666c3e6"
    /// ```
    /// 
    public func createKey(seed: String? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_createKey", params:seed == nil ? RPCObject.none : RPCObject( seed! ), convertWith: toString )
    }

    /// The sign method calculates an Ethereum specific signature with: 
    /// 
    /// ```js
    /// sign(keccak256("\x19Ethereum Signed Message:\n" + len(message) + message))).
    /// ```
    /// 
    /// By adding a prefix to the message makes the calculated signature recognisable as an Ethereum specific signature. This prevents misuse where a malicious DApp can sign arbitrary data (e.g. transaction) and use the signature to impersonate the victim.
    /// 
    /// For the address to sign a signer must be registered.
    /// 
    /// - Parameter account : the account to sign with
    /// - Parameter message : the message to sign
    /// - Returns: the signature (65 bytes) for the given message.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// AccountAPI(in3).sign(account: "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83", message: "0xdeadbeaf") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xa3f20717a250c2b0b729b7e5becbff67fdaef7e0699da4de7ca5895b02a170a12d887fd3b17b\
    /// //          fdce3481f10bea41f45ba9f709d39ce8325427b57afcfc994cee1b"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func sign(account: String, message: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_sign", params:RPCObject( account), RPCObject( message), convertWith: toString )
    }

    /// Signs a transaction that can be submitted to the network at a later time using with eth_sendRawTransaction.
    /// - Parameter tx : transaction to sign
    /// - Returns: the raw signed transaction
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// AccountAPI(in3).signTransaction(tx: AccountTransaction(data: "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675", from: "0xb60e8dd61c5d32be8058bb8eb970870f07233155", gas: "0x76c0", gasPrice: "0x9184e72a000", to: "0xd46e8dd67c5d32be8058bb8eb970870f07244567", value: "0x9184e72a")) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xa3f20717a250c2b0b729b7e5becbff67fdaef7e0699da4de7ca5895b02a170a12d887fd3b17b\
    /// //          fdce3481f10bea41f45ba9f709d39ce8325427b57afcfc994cee1b"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func signTransaction(tx: AccountTransaction) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_signTransaction", params:RPCObject( tx.toRPCDict()), convertWith: toString )
    }

    /// adds a raw private key as signer, which allows signing transactions.
    /// - Parameter pk : the 32byte long private key as hex string.
    /// - Returns: the address of given key.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try AccountAPI(in3).addRawKey(pk: "0x1234567890123456789012345678901234567890123456789012345678901234")
    /// // result = "0x2e988a386a799f506693793c6a5af6b54dfaabfb"
    /// ```
    /// 
    public func addRawKey(pk: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "in3_addRawKey", params:RPCObject( pk), convertWith: toString )
    }

    /// returns a array of account-addresss the incubed client is able to sign with. 
    /// 
    /// In order to add keys, you can use [in3_addRawKey](#in3-addrawkey) or configure them in the config. The result also contains the addresses of any signer signer-supporting the `PLGN_ACT_SIGN_ACCOUNT` action.
    /// 
    /// - Returns: the array of addresses of all registered signers.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try AccountAPI(in3).accounts()
    /// // result = 
    /// //          - "0x2e988a386a799f506693793c6a5af6b54dfaabfb"
    /// //          - "0x93793c6a5af6b54dfaabfb2e988a386a799f5066"
    /// ```
    /// 
    public func accounts() throws ->  [String] {
        return try execLocalAndConvert(in3: in3, method: "eth_accounts", convertWith: { try toArray($0,$1)!.map({ try toString($0, false)! }) } )
    }


}
/// the extracted public key and address
public struct AccountEcrecover {
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
    /// initialize the AccountEcrecover
    ///
    /// - Parameter publicKey : the public Key of the signer (64 bytes)
    /// - Parameter address : the address
    public init(publicKey: String, address: String) {
        self.publicKey = publicKey
        self.address = address
    }
}

/// the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
public struct AccountTransaction {
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
    /// initialize the AccountTransaction
    ///
    /// - Parameter to : receipient of the transaction.
    /// - Parameter from : sender of the address (if not sepcified, the first signer will be the sender)
    /// - Parameter value : value in wei to send
    /// - Parameter gas : the gas to be send along
    /// - Parameter gasPrice : the price in wei for one gas-unit. If not specified it will be fetched using `eth_gasPrice`
    /// - Parameter nonce : the current nonce of the sender. If not specified it will be fetched using `eth_getTransactionCount`
    /// - Parameter data : the data-section of the transaction
    public init(to: String, from: String, value: UInt256? = nil, gas: UInt64? = nil, gasPrice: UInt64? = nil, nonce: UInt64? = nil, data: String? = nil) {
        self.to = to
        self.from = from
        self.value = value
        self.gas = gas
        self.gasPrice = gasPrice
        self.nonce = nonce
        self.data = data
    }
}

/// the signature
public struct AccountSignData {
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
    /// initialize the AccountSignData
    ///
    /// - Parameter message : original message used
    /// - Parameter messageHash : the hash the signature is based on
    /// - Parameter signature : the signature (65 bytes)
    /// - Parameter r : the x-value of the EC-Point
    /// - Parameter s : the y-value of the EC-Point
    /// - Parameter v : the recovery value (0|1) + 27
    public init(message: String, messageHash: String, signature: String, r: String, s: String, v: String) {
        self.message = message
        self.messageHash = messageHash
        self.signature = signature
        self.r = r
        self.s = s
        self.v = v
    }
}