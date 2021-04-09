/// this is generated file don't edit it manually!

import Foundation

/// *Important: This feature is still experimental and not considered stable yet. In order to use it, you need to set the experimental-flag (-x on the comandline or `"experimental":true`!*
/// 
/// the zksync-plugin is able to handle operations to use [zksync](https://zksync.io/) like deposit transfer or withdraw. Also see the #in3-config on how to configure the zksync-server or account.
/// 
/// Also in order to sign messages you need to set a signer!
/// 
/// All zksync-methods can be used with `zksync_` or `zk_` prefix. 
/// 
public class Zksync {
    internal var in3: In3

    /// initialiazes the Zksync API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// returns the contract address
    /// - Returns: fetches the contract addresses from the zksync server. This request also caches them and will return the results from cahe if available.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).contractAddress() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          govContract: "0x34460C0EB5074C29A9F6FE13b8e7E23A0D08aF01"
    /// //          mainContract: "0xaBEA9132b05A70803a4E85094fD0e1800777fBEF"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func contractAddress() -> Future<ZksyncContractAddress> {
        return execAndConvert(in3: in3, method: "zksync_contract_address", convertWith: { try ZksyncContractAddress($0,$1) } )
    }

    /// returns the list of all available tokens
    /// - Returns: a array of tokens-definitions. This request also caches them and will return the results from cahe if available.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).tokens() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          BAT:
    /// //            address: "0x0d8775f648430679a709e98d2b0cb6250d2887ef"
    /// //            decimals: 18
    /// //            id: 8
    /// //            symbol: BAT
    /// //          BUSD:
    /// //            address: "0x4fabb145d64652a948d72533023f6e7a623c7c53"
    /// //            decimals: 18
    /// //            id: 6
    /// //            symbol: BUSD
    /// //          DAI:
    /// //            address: "0x6b175474e89094c44da98b954eedeac495271d0f"
    /// //            decimals: 18
    /// //            id: 1
    /// //            symbol: DAI
    /// //          ETH:
    /// //            address: "0x0000000000000000000000000000000000000000"
    /// //            decimals: 18
    /// //            id: 0
    /// //            symbol: ETH
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func tokens() -> Future<[String:ZksyncTokens]> {
        return execAndConvert(in3: in3, method: "zksync_tokens", convertWith: { try toObject($0,$1)!.mapValues({ try ZksyncTokens($0,false)! }) } )
    }

    /// returns account_info from the server
    /// - Parameter address : the account-address. if not specified, the client will try to use its own address based on the signer config.
    /// - Returns: the current state of the requested account.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).accountInfo() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          address: "0x3b2a1bd631d9d7b17e87429a8e78dbbd9b4de292"
    /// //          committed:
    /// //            balances: {}
    /// //            nonce: 0
    /// //            pubKeyHash: sync:0000000000000000000000000000000000000000
    /// //          depositing:
    /// //            balances: {}
    /// //          id: null
    /// //          verified:
    /// //            balances: {}
    /// //            nonce: 0
    /// //            pubKeyHash: sync:0000000000000000000000000000000000000000
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func accountInfo(address: String? = nil) -> Future<ZksyncAccountInfo> {
        return execAndConvert(in3: in3, method: "zksync_account_info", params:address == nil ? RPCObject.none : RPCObject( address! ), convertWith: { try ZksyncAccountInfo($0,$1) } )
    }

    /// returns the state or receipt of the the zksync-tx
    /// - Parameter tx : the txHash of the send tx
    /// - Returns: the current state of the requested tx.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).txInfo(tx: "sync-tx:e41d2489571d322189246dafa5ebde1f4699f498000000000000000000000000") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          block: null
    /// //          executed: false
    /// //          failReason: null
    /// //          success: null
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func txInfo(tx: String) -> Future<ZksyncTxInfo> {
        return execAndConvert(in3: in3, method: "zksync_tx_info", params:RPCObject( tx), convertWith: { try ZksyncTxInfo($0,$1) } )
    }

    /// sets the signerkey based on the current pk or as configured in the config.
    /// You can specify the key by either
    /// - setting a signer ( the sync key will be derrived through a signature )
    /// - setting the seed directly ( `sync_key` in the config)
    /// - setting the `musig_pub_keys` to generate the pubKeyHash based on them
    /// - setting the `create2` options and the sync-key will generate the account based on the pubKeyHash
    /// 
    /// 
    /// we support 3 different signer types (`signer_type` in the `zksync` config) :
    /// 
    /// 1. `pk` - Simple Private Key
    ///     If a signer is set (for example by setting the pk), incubed will derrive the sync-key through a signature and use it
    /// 2. `contract` - Contract Signature
    ///     In this case a preAuth-tx will be send on L1 using the signer. If this contract is a mutisig, you should make sure, you have set the account explicitly in the config and also activate the multisig-plugin, so the transaction will be send through the multisig.
    /// 3. `create2` - Create2 based Contract
    /// 
    /// - Parameter token : the token to pay the gas (either the symbol or the address)
    /// - Returns: the pubKeyHash, if it was executed successfully
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).setKey(token: "eth") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = sync:e41d2489571d322189246dafa5ebde1f4699f498
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func setKey(token: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_set_key", params:RPCObject( token), convertWith: toString )
    }

    /// returns the current PubKeyHash based on the configuration set.
    /// - Parameter pubKey : the packed public key to hash ( if given the hash is build based on the given hash, otherwise the hash is based on the config)
    /// - Returns: the pubKeyHash
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try ZksyncAPI(in3).pubkeyhash()
    /// // result = sync:4dcd9bb4463121470c7232efb9ff23ec21398e58
    /// ```
    /// 
    public func pubkeyhash(pubKey: String? = nil) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "zksync_pubkeyhash", params:pubKey == nil ? RPCObject.none : RPCObject( pubKey! ), convertWith: toString )
    }

    /// returns the current packed PubKey based on the config set.
    /// 
    /// If the config contains public keys for musig-signatures, the keys will be aggregated, otherwise the pubkey will be derrived from the signing key set.
    /// 
    /// - Returns: the pubKey
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try ZksyncAPI(in3).pubkey()
    /// // result = "0xfca80a469dbb53f8002eb1e2569d66f156f0df24d71bd589432cc7bc647bfc04"
    /// ```
    /// 
    public func pubkey() throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "zksync_pubkey", convertWith: toString )
    }

    /// returns the address of the account used.
    /// - Returns: the account used.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try ZksyncAPI(in3).accountAddress()
    /// // result = "0x3b2a1bd631d9d7b17e87429a8e78dbbd9b4de292"
    /// ```
    /// 
    public func accountAddress() throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "zksync_account_address", convertWith: toString )
    }

    /// returns the schnorr musig signature based on the current config. 
    /// 
    /// This also supports signing with multiple keys. In this case the configuration needs to sets the urls of the other keys, so the client can then excange all data needed in order to create the combined signature. 
    /// when exchanging the data with other keys, all known data will be send using `zk_sign` as method, but instead of the raw message a object with those data will be passed.
    /// 
    /// - Parameter message : the message to sign
    /// - Returns: The return value are 96 bytes of signature:
    /// - `[0...32]` packed public key
    /// - `[32..64]` r-value
    /// - `[64..96]` s-value
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).sign(message: "0xaabbccddeeff") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xfca80a469dbb53f8002eb1e2569d66f156f0df24d71bd589432cc7bc647bfc0493f69034c398\
    /// //          0e7352741afa6c171b8e18355e41ed7427f6e706f8432e32e920c3e61e6c3aa00cfe0c202c29a31\
    /// //          b69cd0910a432156a0977c3a5baa404547e01"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func sign(message: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_sign", params:RPCObject( message), convertWith: toString )
    }

    /// returns 0 or 1 depending on the successfull verification of the signature.
    /// 
    /// if the `musig_pubkeys` are set it will also verify against the given public keys list. 
    /// 
    /// - Parameter message : the message which was supposed to be signed
    /// - Parameter signature : the signature (96 bytes)
    /// - Returns: 1 if the signature(which contains the pubkey as the first 32bytes) matches the message.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try ZksyncAPI(in3).verify(message: "0xaabbccddeeff", signature: "0xfca80a469dbb53f8002eb1e2569d66f156f0df24d71bd589432cc7bc647bfc0493f69034c3980e7352741afa6c171b8e18355e41ed7427f6e706f8432e32e920c3e61e6c3aa00cfe0c202c29a31b69cd0910a432156a0977c3a5baa404547e01")
    /// // result = 1
    /// ```
    /// 
    public func verify(message: String, signature: String) throws ->  Int {
        return try execLocalAndConvert(in3: in3, method: "zksync_verify", params:RPCObject( message), RPCObject( signature), convertWith: toInt )
    }

    /// returns the state or receipt of the the PriorityOperation
    /// - Parameter opId : the opId of a layer-operstion (like depositing)
    public func ethopInfo(opId: UInt64) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_ethop_info", params:RPCObject( String(format: "0x%1x", opId)), convertWith: toString )
    }

    /// returns current token-price
    /// - Parameter token : Symbol or address of the token
    /// - Returns: the token price
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).getTokenPrice(token: "WBTC") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 11320.002167
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTokenPrice(token: String) -> Future<Double> {
        return execAndConvert(in3: in3, method: "zksync_get_token_price", params:RPCObject( token), convertWith: toDouble )
    }

    /// calculates the fees for a transaction.
    /// - Parameter txType : The Type of the transaction "Withdraw" or "Transfer"
    /// - Parameter address : the address of the receipient
    /// - Parameter token : the symbol or address of the token to pay
    /// - Returns: the fees split up into single values
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).getTxFee(txType: "Transfer", address: "0xabea9132b05a70803a4e85094fd0e1800777fbef", token: "BAT") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          feeType: TransferToNew
    /// //          gasFee: "47684047990828528"
    /// //          gasPriceWei: "116000000000"
    /// //          gasTxAmount: "350"
    /// //          totalFee: "66000000000000000"
    /// //          zkpFee: "18378682992117666"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTxFee(txType: String, address: String, token: String) -> Future<ZksyncTxFee> {
        return execAndConvert(in3: in3, method: "zksync_get_tx_fee", params:RPCObject( txType), RPCObject( address), RPCObject( token), convertWith: { try ZksyncTxFee($0,$1) } )
    }

    /// returns private key used for signing zksync-transactions
    /// - Returns: the raw private key configured based on the signers seed
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).syncKey() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x019125314fda133d5bf62cb454ee8c60927d55b68eae8b8b8bd13db814389cd6"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func syncKey() -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_sync_key", convertWith: toString )
    }

    /// sends a deposit-transaction and returns the opId, which can be used to tradck progress.
    /// - Parameter amount : the value to deposit in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter approveDepositAmountForERC20 : if true and in case of an erc20-token, the client will send a approve transaction first, otherwise it is expected to be already approved.
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the opId. You can use `zksync_ethop_info` to follow the state-changes.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).deposit(amount: 1000, token: "WBTC") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 74
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func deposit(amount: UInt256, token: String, approveDepositAmountForERC20: Bool? = nil, account: String? = nil) -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "zksync_deposit", params:RPCObject( amount), RPCObject( token), approveDepositAmountForERC20 == nil ? RPCObject.none : RPCObject( approveDepositAmountForERC20! ), account == nil ? RPCObject.none : RPCObject( account! ), convertWith: toUInt64 )
    }

    /// sends a zksync-transaction and returns data including the transactionHash.
    /// - Parameter to : the receipient of the tokens
    /// - Parameter amount : the value to transfer in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the transactionHash. use `zksync_tx_info` to check the progress.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).transfer(to: 9.814684447173249e+47, amount: 100, token: "WBTC") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x58ba1537596739d990a33e4fba3a6fb4e0d612c5de30843a2c415dd1e5edcef1"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func transfer(to: String, amount: UInt256, token: String, account: String? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_transfer", params:RPCObject( to), RPCObject( amount), RPCObject( token), account == nil ? RPCObject.none : RPCObject( account! ), convertWith: toString )
    }

    /// withdraws the amount to the given `ethAddress` for the given token.
    /// - Parameter ethAddress : the receipient of the tokens in L1
    /// - Parameter amount : the value to transfer in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the transactionHash. use `zksync_tx_info` to check the progress.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).withdraw(ethAddress: 9.814684447173249e+47, amount: 100, token: "WBTC") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x58ba1537596739d990a33e4fba3a6fb4e0d612c5de30843a2c415dd1e5edcef1"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func withdraw(ethAddress: String, amount: UInt256, token: String, account: String? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_withdraw", params:RPCObject( ethAddress), RPCObject( amount), RPCObject( token), account == nil ? RPCObject.none : RPCObject( account! ), convertWith: toString )
    }

    /// withdraws all tokens for the specified token as a onchain-transaction. This is useful in case the zksync-server is offline or tries to be malicious.
    /// - Parameter token : the token as symbol or address
    /// - Returns: the transactionReceipt
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// ZksyncAPI(in3).emergencyWithdraw(token: "WBTC") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          blockHash: "0xea6ee1e20d3408ad7f6981cfcc2625d80b4f4735a75ca5b20baeb328e41f0304"
    /// //          blockNumber: "0x8c1e39"
    /// //          contractAddress: null
    /// //          cumulativeGasUsed: "0x2466d"
    /// //          gasUsed: "0x2466d"
    /// //          logs:
    /// //            - address: "0x85ec283a3ed4b66df4da23656d4bf8a507383bca"
    /// //              blockHash: "0xea6ee1e20d3408ad7f6981cfcc2625d80b4f4735a75ca5b20baeb328e41f0304"
    /// //              blockNumber: "0x8c1e39"
    /// //              data: 0x00000000000...
    /// //              logIndex: "0x0"
    /// //              removed: false
    /// //              topics:
    /// //                - "0x9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8"
    /// //                - "0xa2f7689fc12ea917d9029117d32b9fdef2a53462c853462ca86b71b97dd84af6"
    /// //                - "0x55a6ef49ec5dcf6cd006d21f151f390692eedd839c813a150000000000000000"
    /// //              transactionHash: "0x5dc2a9ec73abfe0640f27975126bbaf14624967e2b0b7c2b3a0fb6111f0d3c5e"
    /// //              transactionIndex: "0x0"
    /// //              transactionLogIndex: "0x0"
    /// //              type: mined
    /// //          logsBloom: 0x00000000000000000000200000...
    /// //          root: null
    /// //          status: "0x1"
    /// //          transactionHash: "0x5dc2a9ec73abfe0640f27975126bbaf14624967e2b0b7c2b3a0fb6111f0d3c5e"
    /// //          transactionIndex: "0x0"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func emergencyWithdraw(token: String) -> Future<ZksyncTransactionReceipt> {
        return execAndConvert(in3: in3, method: "zksync_emergency_withdraw", params:RPCObject( token), convertWith: { try ZksyncTransactionReceipt($0,$1) } )
    }

    /// calculate the public key based on multiple public keys signing together using schnorr musig signatures.
    /// - Parameter pubkeys : concatinated packed publickeys of the signers. the length of the bytes must be `num_keys * 32`
    /// - Returns: the compact public Key
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// let result = try ZksyncAPI(in3).aggregatePubkey(pubkeys: "0x0f61bfe164cc43b5a112bfbfb0583004e79dbfafc97a7daad14c5d511fea8e2435065ddd04329ec94be682bf004b03a5a4eeca9bf50a8b8b6023942adc0b3409")
    /// // result = "0x9ce5b6f8db3fbbe66a3bdbd3b4731f19ec27f80ee03ead3c0708798dd949882b"
    /// ```
    /// 
    public func aggregatePubkey(pubkeys: String) throws ->  String {
        return try execLocalAndConvert(in3: in3, method: "zksync_aggregate_pubkey", params:RPCObject( pubkeys), convertWith: toString )
    }


}
/// fetches the contract addresses from the zksync server. This request also caches them and will return the results from cahe if available.
public struct ZksyncContractAddress {
    /// the address of the govement contract
    public var govContract: String

    /// the address of the main contract
    public var mainContract: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        govContract = try toString(obj["govContract"],false)!
        mainContract = try toString(obj["mainContract"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["govContract"] = RPCObject(govContract)
        obj["mainContract"] = RPCObject(mainContract)
        return obj
    }
    /// initialize the ZksyncContractAddress
    ///
    /// - Parameter govContract : the address of the govement contract
    /// - Parameter mainContract : the address of the main contract
    public init(govContract: String, mainContract: String) {
        self.govContract = govContract
        self.mainContract = mainContract
    }
}

/// a array of tokens-definitions. This request also caches them and will return the results from cahe if available.
public struct ZksyncTokens {
    /// the address of the ERC2-Contract or 0x00000..000 in case of the native token (eth)
    public var address: String

    /// decimals to be used when formating it for human readable representation.
    public var decimals: Int

    /// id which will be used when encoding the token.
    public var id: UInt64

    /// symbol for the token
    public var symbol: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        address = try toString(obj["address"],false)!
        decimals = try toInt(obj["decimals"],false)!
        id = try toUInt64(obj["id"],false)!
        symbol = try toString(obj["symbol"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["address"] = RPCObject(address)
        obj["decimals"] = RPCObject(decimals)
        obj["id"] = RPCObject(id)
        obj["symbol"] = RPCObject(symbol)
        return obj
    }
    /// initialize the ZksyncTokens
    ///
    /// - Parameter address : the address of the ERC2-Contract or 0x00000..000 in case of the native token (eth)
    /// - Parameter decimals : decimals to be used when formating it for human readable representation.
    /// - Parameter id : id which will be used when encoding the token.
    /// - Parameter symbol : symbol for the token
    public init(address: String, decimals: Int, id: UInt64, symbol: String) {
        self.address = address
        self.decimals = decimals
        self.id = id
        self.symbol = symbol
    }
}

/// the current state of the requested account.
public struct ZksyncAccountInfo {
    /// the address of the account
    public var address: String

    /// the state of the zksync operator after executing transactions successfully, but not not verified on L1 yet.
    public var commited: ZksyncCommited

    /// the state of all depositing-tx.
    public var depositing: ZksyncDepositing

    /// the assigned id of the account, which will be used when encoding it into the rollup.
    public var id: UInt64

    /// the state after the rollup was verified in L1.
    public var verified: ZksyncVerified

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        address = try toString(obj["address"],false)!
        commited = try ZksyncCommited(obj["commited"],false)!
        depositing = try ZksyncDepositing(obj["depositing"],false)!
        id = try toUInt64(obj["id"],false)!
        verified = try ZksyncVerified(obj["verified"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["address"] = RPCObject(address)
        obj["id"] = RPCObject(id)
        return obj
    }
    /// initialize the ZksyncAccountInfo
    ///
    /// - Parameter address : the address of the account
    /// - Parameter commited : the state of the zksync operator after executing transactions successfully, but not not verified on L1 yet.
    /// - Parameter depositing : the state of all depositing-tx.
    /// - Parameter id : the assigned id of the account, which will be used when encoding it into the rollup.
    /// - Parameter verified : the state after the rollup was verified in L1.
    public init(address: String, commited: ZksyncCommited, depositing: ZksyncDepositing, id: UInt64, verified: ZksyncVerified) {
        self.address = address
        self.commited = commited
        self.depositing = depositing
        self.id = id
        self.verified = verified
    }
}

/// the state of the zksync operator after executing transactions successfully, but not not verified on L1 yet.
public struct ZksyncCommited {
    /// the token-balance
    public var balances: [String:UInt256]

    /// the nonce or transaction count.
    public var nonce: UInt64

    /// the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public var pubKeyHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt256($0,false)! })
        nonce = try toUInt64(obj["nonce"],false)!
        pubKeyHash = try toString(obj["pubKeyHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["nonce"] = RPCObject(nonce)
        obj["pubKeyHash"] = RPCObject(pubKeyHash)
        return obj
    }
    /// initialize the ZksyncCommited
    ///
    /// - Parameter balances : the token-balance
    /// - Parameter nonce : the nonce or transaction count.
    /// - Parameter pubKeyHash : the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public init(balances: [String:UInt256], nonce: UInt64, pubKeyHash: String) {
        self.balances = balances
        self.nonce = nonce
        self.pubKeyHash = pubKeyHash
    }
}

/// the state of all depositing-tx.
public struct ZksyncDepositing {
    /// the token-values.
    public var balances: [String:UInt256]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt256($0,false)! })
    }

    /// initialize the ZksyncDepositing
    ///
    /// - Parameter balances : the token-values.
    public init(balances: [String:UInt256]) {
        self.balances = balances
    }
}

/// the state after the rollup was verified in L1.
public struct ZksyncVerified {
    /// the token-balances.
    public var balances: [String:UInt256]

    /// the nonce or transaction count.
    public var nonce: UInt64

    /// the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public var pubKeyHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt256($0,false)! })
        nonce = try toUInt64(obj["nonce"],false)!
        pubKeyHash = try toString(obj["pubKeyHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["nonce"] = RPCObject(nonce)
        obj["pubKeyHash"] = RPCObject(pubKeyHash)
        return obj
    }
    /// initialize the ZksyncVerified
    ///
    /// - Parameter balances : the token-balances.
    /// - Parameter nonce : the nonce or transaction count.
    /// - Parameter pubKeyHash : the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public init(balances: [String:UInt256], nonce: UInt64, pubKeyHash: String) {
        self.balances = balances
        self.nonce = nonce
        self.pubKeyHash = pubKeyHash
    }
}

/// the current state of the requested tx.
public struct ZksyncTxInfo {
    /// the blockNumber containing the tx or `null` if still pending
    public var block: UInt64

    /// true, if the tx has been executed by the operator. If false it is still in the txpool of the operator.
    public var executed: Bool

    /// if executed, this property marks the success of the tx.
    public var success: Bool

    /// if executed and failed this will include an error message
    public var failReason: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        block = try toUInt64(obj["block"],false)!
        executed = try toBool(obj["executed"],false)!
        success = try toBool(obj["success"],false)!
        failReason = try toString(obj["failReason"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["block"] = RPCObject(block)
        obj["executed"] = RPCObject(executed)
        obj["success"] = RPCObject(success)
        obj["failReason"] = RPCObject(failReason)
        return obj
    }
    /// initialize the ZksyncTxInfo
    ///
    /// - Parameter block : the blockNumber containing the tx or `null` if still pending
    /// - Parameter executed : true, if the tx has been executed by the operator. If false it is still in the txpool of the operator.
    /// - Parameter success : if executed, this property marks the success of the tx.
    /// - Parameter failReason : if executed and failed this will include an error message
    public init(block: UInt64, executed: Bool, success: Bool, failReason: String) {
        self.block = block
        self.executed = executed
        self.success = success
        self.failReason = failReason
    }
}

/// the fees split up into single values
public struct ZksyncTxFee {
    /// Type of the transaaction
    public var feeType: String

    /// the gas for the core-transaction
    public var gasFee: UInt64

    /// current gasPrice
    public var gasPriceWei: UInt64

    /// gasTxAmount
    public var gasTxAmount: UInt64

    /// total of all fees needed to pay in order to execute the transaction
    public var totalFee: UInt64

    /// zkpFee
    public var zkpFee: UInt64

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        feeType = try toString(obj["feeType"],false)!
        gasFee = try toUInt64(obj["gasFee"],false)!
        gasPriceWei = try toUInt64(obj["gasPriceWei"],false)!
        gasTxAmount = try toUInt64(obj["gasTxAmount"],false)!
        totalFee = try toUInt64(obj["totalFee"],false)!
        zkpFee = try toUInt64(obj["zkpFee"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["feeType"] = RPCObject(feeType)
        obj["gasFee"] = RPCObject(gasFee)
        obj["gasPriceWei"] = RPCObject(gasPriceWei)
        obj["gasTxAmount"] = RPCObject(gasTxAmount)
        obj["totalFee"] = RPCObject(totalFee)
        obj["zkpFee"] = RPCObject(zkpFee)
        return obj
    }
    /// initialize the ZksyncTxFee
    ///
    /// - Parameter feeType : Type of the transaaction
    /// - Parameter gasFee : the gas for the core-transaction
    /// - Parameter gasPriceWei : current gasPrice
    /// - Parameter gasTxAmount : gasTxAmount
    /// - Parameter totalFee : total of all fees needed to pay in order to execute the transaction
    /// - Parameter zkpFee : zkpFee
    public init(feeType: String, gasFee: UInt64, gasPriceWei: UInt64, gasTxAmount: UInt64, totalFee: UInt64, zkpFee: UInt64) {
        self.feeType = feeType
        self.gasFee = gasFee
        self.gasPriceWei = gasPriceWei
        self.gasTxAmount = gasTxAmount
        self.totalFee = totalFee
        self.zkpFee = zkpFee
    }
}

/// the transactionReceipt
public struct ZksyncTransactionReceipt {
    /// the blockNumber
    public var blockNumber: UInt64

    /// blockhash if ther containing block
    public var blockHash: String

    /// the deployed contract in case the tx did deploy a new contract
    public var contractAddress: String

    /// gas used for all transaction up to this one in the block
    public var cumulativeGasUsed: UInt64

    /// gas used by this transaction.
    public var gasUsed: UInt64

    /// array of events created during execution of the tx
    public var logs: ZksyncLogs

    /// bloomfilter used to detect events for `eth_getLogs`
    public var logsBloom: String

    /// error-status of the tx.  0x1 = success 0x0 = failure
    public var status: Int

    /// requested transactionHash
    public var transactionHash: String

    /// transactionIndex within the containing block.
    public var transactionIndex: Int

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        blockHash = try toString(obj["blockHash"],false)!
        contractAddress = try toString(obj["contractAddress"],false)!
        cumulativeGasUsed = try toUInt64(obj["cumulativeGasUsed"],false)!
        gasUsed = try toUInt64(obj["gasUsed"],false)!
        logs = try ZksyncLogs(obj["logs"],false)!
        logsBloom = try toString(obj["logsBloom"],false)!
        status = try toInt(obj["status"],false)!
        transactionHash = try toString(obj["transactionHash"],false)!
        transactionIndex = try toInt(obj["transactionIndex"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["blockNumber"] = RPCObject(blockNumber)
        obj["blockHash"] = RPCObject(blockHash)
        obj["contractAddress"] = RPCObject(contractAddress)
        obj["cumulativeGasUsed"] = RPCObject(cumulativeGasUsed)
        obj["gasUsed"] = RPCObject(gasUsed)
        obj["logsBloom"] = RPCObject(logsBloom)
        obj["status"] = RPCObject(status)
        obj["transactionHash"] = RPCObject(transactionHash)
        obj["transactionIndex"] = RPCObject(transactionIndex)
        return obj
    }
    /// initialize the ZksyncTransactionReceipt
    ///
    /// - Parameter blockNumber : the blockNumber
    /// - Parameter blockHash : blockhash if ther containing block
    /// - Parameter contractAddress : the deployed contract in case the tx did deploy a new contract
    /// - Parameter cumulativeGasUsed : gas used for all transaction up to this one in the block
    /// - Parameter gasUsed : gas used by this transaction.
    /// - Parameter logs : array of events created during execution of the tx
    /// - Parameter logsBloom : bloomfilter used to detect events for `eth_getLogs`
    /// - Parameter status : error-status of the tx.  0x1 = success 0x0 = failure
    /// - Parameter transactionHash : requested transactionHash
    /// - Parameter transactionIndex : transactionIndex within the containing block.
    public init(blockNumber: UInt64, blockHash: String, contractAddress: String, cumulativeGasUsed: UInt64, gasUsed: UInt64, logs: ZksyncLogs, logsBloom: String, status: Int, transactionHash: String, transactionIndex: Int) {
        self.blockNumber = blockNumber
        self.blockHash = blockHash
        self.contractAddress = contractAddress
        self.cumulativeGasUsed = cumulativeGasUsed
        self.gasUsed = gasUsed
        self.logs = logs
        self.logsBloom = logsBloom
        self.status = status
        self.transactionHash = transactionHash
        self.transactionIndex = transactionIndex
    }
}

/// array of events created during execution of the tx
public struct ZksyncLogs {
    /// the address triggering the event.
    public var address: String

    /// the blockNumber
    public var blockNumber: UInt64

    /// blockhash if ther containing block
    public var blockHash: String

    /// abi-encoded data of the event (all non indexed fields)
    public var data: String

    /// the index of the even within the block.
    public var logIndex: Int

    /// the reorg-status of the event.
    public var removed: Bool

    /// array of 32byte-topics of the indexed fields.
    public var topics: [String]

    /// requested transactionHash
    public var transactionHash: String

    /// transactionIndex within the containing block.
    public var transactionIndex: Int

    /// index of the event within the transaction.
    public var transactionLogIndex: Int

    /// mining-status
    public var type: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        address = try toString(obj["address"],false)!
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        blockHash = try toString(obj["blockHash"],false)!
        data = try toString(obj["data"],false)!
        logIndex = try toInt(obj["logIndex"],false)!
        removed = try toBool(obj["removed"],false)!
        topics = try toArray(obj["topics"])!.map({ try toString($0,false)! })
        transactionHash = try toString(obj["transactionHash"],false)!
        transactionIndex = try toInt(obj["transactionIndex"],false)!
        transactionLogIndex = try toInt(obj["transactionLogIndex"],false)!
        type = try toString(obj["type"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["address"] = RPCObject(address)
        obj["blockNumber"] = RPCObject(blockNumber)
        obj["blockHash"] = RPCObject(blockHash)
        obj["data"] = RPCObject(data)
        obj["logIndex"] = RPCObject(logIndex)
        obj["removed"] = RPCObject(removed)
        obj["transactionHash"] = RPCObject(transactionHash)
        obj["transactionIndex"] = RPCObject(transactionIndex)
        obj["transactionLogIndex"] = RPCObject(transactionLogIndex)
        obj["type"] = RPCObject(type)
        return obj
    }
    /// initialize the ZksyncLogs
    ///
    /// - Parameter address : the address triggering the event.
    /// - Parameter blockNumber : the blockNumber
    /// - Parameter blockHash : blockhash if ther containing block
    /// - Parameter data : abi-encoded data of the event (all non indexed fields)
    /// - Parameter logIndex : the index of the even within the block.
    /// - Parameter removed : the reorg-status of the event.
    /// - Parameter topics : array of 32byte-topics of the indexed fields.
    /// - Parameter transactionHash : requested transactionHash
    /// - Parameter transactionIndex : transactionIndex within the containing block.
    /// - Parameter transactionLogIndex : index of the event within the transaction.
    /// - Parameter type : mining-status
    public init(address: String, blockNumber: UInt64, blockHash: String, data: String, logIndex: Int, removed: Bool, topics: [String], transactionHash: String, transactionIndex: Int, transactionLogIndex: Int, type: String) {
        self.address = address
        self.blockNumber = blockNumber
        self.blockHash = blockHash
        self.data = data
        self.logIndex = logIndex
        self.removed = removed
        self.topics = topics
        self.transactionHash = transactionHash
        self.transactionIndex = transactionIndex
        self.transactionLogIndex = transactionLogIndex
        self.type = type
    }
}