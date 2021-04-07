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
public class ZksyncAPI {
    internal var in3: In3

    /// initialiazes the Zksync API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// returns the contract address
    /// - Returns: fetches the contract addresses from the zksync server. This request also caches them and will return the results from cahe if available.
    public func contractAddress() -> Future<ZksyncContractAddress> {
        return execAndConvert(in3: in3, method: "zksync_contract_address", convertWith: { try ZksyncContractAddress($0,$1) } )
    }

    /// returns the list of all available tokens
    /// - Returns: a array of tokens-definitions. This request also caches them and will return the results from cahe if available.
    public func tokens() -> Future<ZksyncTokens> {
        return execAndConvert(in3: in3, method: "zksync_tokens", convertWith: { try ZksyncTokens($0,$1) } )
    }

    /// returns account_info from the server
    /// - Parameter address : the account-address. if not specified, the client will try to use its own address based on the signer config.
    /// - Returns: the current state of the requested account.
    public func accountInfo(address: String? = nil) -> Future<ZksyncAccountInfo> {
        return execAndConvert(in3: in3, method: "zksync_account_info",params:address == nil ? RPCObject.none : RPCObject(address!), convertWith: { try ZksyncAccountInfo($0,$1) } )
    }

    /// returns the state or receipt of the the zksync-tx
    /// - Parameter tx : the txHash of the send tx
    /// - Returns: the current state of the requested tx.
    public func txInfo(tx: String) -> Future<ZksyncTxInfo> {
        return execAndConvert(in3: in3, method: "zksync_tx_info",params: RPCObject(tx), convertWith: { try ZksyncTxInfo($0,$1) } )
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
    public func setKey(token: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_set_key",params: RPCObject(token), convertWith: toString )
    }

    /// returns the current PubKeyHash based on the configuration set.
    /// - Parameter pubKey : the packed public key to hash ( if given the hash is build based on the given hash, otherwise the hash is based on the config)
    /// - Returns: the pubKeyHash
    public func pubkeyhash(pubKey: String? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_pubkeyhash",params:pubKey == nil ? RPCObject.none : RPCObject(pubKey!), convertWith: toString )
    }

    /// returns the current packed PubKey based on the config set.
    /// 
    /// If the config contains public keys for musig-signatures, the keys will be aggregated, otherwise the pubkey will be derrived from the signing key set.
    /// 
    /// - Returns: the pubKey
    public func pubkey() -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_pubkey", convertWith: toString )
    }

    /// returns the address of the account used.
    /// - Returns: the account used.
    public func accountAddress() -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_account_address", convertWith: toString )
    }

    /// returns the schnorr musig signature based on the current config. 
    /// 
    /// This also supports signing with multiple keys. In this case the configuration needs to sets the urls of the other keys, so the client can then excange all data needed in order to create the combined signature. 
    /// when exchanging the data with other keys, all known data will be send using `zk_sign` as method, but instead of the raw message a object with those data will be passed.
    /// 
    /// - Parameter message : the message to sign
    /// - Returns: The return value are 96 bytes of signature:    /// - `[0...32]` packed public key    /// - `[32..64]` r-value    /// - `[64..96]` s-value    /// 
    public func sign(message: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_sign",params: RPCObject(message), convertWith: toString )
    }

    /// returns 0 or 1 depending on the successfull verification of the signature.
    /// 
    /// if the `musig_pubkeys` are set it will also verify against the given public keys list. 
    /// 
    /// - Parameter message : the message which was supposed to be signed
    /// - Parameter signature : the signature (96 bytes)
    /// - Returns: 1 if the signature(which contains the pubkey as the first 32bytes) matches the message.
    public func verify(message: String, signature: String) -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "zksync_verify",params: RPCObject(message), RPCObject(signature), convertWith: toUInt64 )
    }

    /// returns the state or receipt of the the PriorityOperation
    /// - Parameter opId : the opId of a layer-operstion (like depositing)
    public func ethopInfo(opId: UInt64) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_ethop_info",params: RPCObject(opId), convertWith: toString )
    }

    /// returns current token-price
    /// - Parameter token : Symbol or address of the token
    /// - Returns: the token price
    public func getTokenPrice(token: String) -> Future<Double> {
        return execAndConvert(in3: in3, method: "zksync_get_token_price",params: RPCObject(token), convertWith: toDouble )
    }

    /// calculates the fees for a transaction.
    /// - Parameter txType : The Type of the transaction "Withdraw" or "Transfer"
    /// - Parameter address : the address of the receipient
    /// - Parameter token : the symbol or address of the token to pay
    /// - Returns: the fees split up into single values
    public func getTxFee(txType: String, address: String, token: String) -> Future<ZksyncTxFee> {
        return execAndConvert(in3: in3, method: "zksync_get_tx_fee",params: RPCObject(txType), RPCObject(address), RPCObject(token), convertWith: { try ZksyncTxFee($0,$1) } )
    }

    /// returns private key used for signing zksync-transactions
    /// - Returns: the raw private key configured based on the signers seed
    public func syncKey() -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_sync_key", convertWith: toString )
    }

    /// sends a deposit-transaction and returns the opId, which can be used to tradck progress.
    /// - Parameter amount : the value to deposit in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter approveDepositAmountForERC20 : if true and in case of an erc20-token, the client will send a approve transaction first, otherwise it is expected to be already approved.
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the opId. You can use `zksync_ethop_info` to follow the state-changes.
    public func deposit(amount: UInt64, token: String, approveDepositAmountForERC20: Bool? = nil, account: String? = nil) -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "zksync_deposit",params: RPCObject(amount), RPCObject(token),approveDepositAmountForERC20 == nil ? RPCObject.none : RPCObject(approveDepositAmountForERC20!),account == nil ? RPCObject.none : RPCObject(account!), convertWith: toUInt64 )
    }

    /// sends a zksync-transaction and returns data including the transactionHash.
    /// - Parameter to : the receipient of the tokens
    /// - Parameter amount : the value to transfer in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the transactionHash. use `zksync_tx_info` to check the progress.
    public func transfer(to: String, amount: UInt64, token: String, account: String? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_transfer",params: RPCObject(to), RPCObject(amount), RPCObject(token),account == nil ? RPCObject.none : RPCObject(account!), convertWith: toString )
    }

    /// withdraws the amount to the given `ethAddress` for the given token.
    /// - Parameter ethAddress : the receipient of the tokens in L1
    /// - Parameter amount : the value to transfer in wei (or smallest token unit)
    /// - Parameter token : the token as symbol or address
    /// - Parameter account : address of the account to send the tx from. if not specified, the first available signer will be used.
    /// - Returns: the transactionHash. use `zksync_tx_info` to check the progress.
    public func withdraw(ethAddress: String, amount: UInt64, token: String, account: String? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_withdraw",params: RPCObject(ethAddress), RPCObject(amount), RPCObject(token),account == nil ? RPCObject.none : RPCObject(account!), convertWith: toString )
    }

    /// withdraws all tokens for the specified token as a onchain-transaction. This is useful in case the zksync-server is offline or tries to be malicious.
    /// - Parameter token : the token as symbol or address
    /// - Returns: the transactionReceipt
    public func emergencyWithdraw(token: String) -> Future<ZksyncTransactionReceipt> {
        return execAndConvert(in3: in3, method: "zksync_emergency_withdraw",params: RPCObject(token), convertWith: { try ZksyncTransactionReceipt($0,$1) } )
    }

    /// calculate the public key based on multiple public keys signing together using schnorr musig signatures.
    /// - Parameter pubkeys : concatinated packed publickeys of the signers. the length of the bytes must be `num_keys * 32`
    /// - Returns: the compact public Key
    public func aggregatePubkey(pubkeys: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "zksync_aggregate_pubkey",params: RPCObject(pubkeys), convertWith: toString )
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
}

/// a array of tokens-definitions. This request also caches them and will return the results from cahe if available.
public struct ZksyncTokens {
    /// the address of the ERC2-Contract or 0x00000..000 in case of the native token (eth)
    public var address: String

    /// decimals to be used when formating it for human readable representation.
    public var decimals: UInt64

    /// id which will be used when encoding the token.
    public var id: UInt64

    /// symbol for the token
    public var symbol: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        address = try toString(obj["address"],false)!
        decimals = try toUInt64(obj["decimals"],false)!
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
}

/// the state of the zksync operator after executing transactions successfully, but not not verified on L1 yet.
public struct ZksyncCommited {
    /// the token-balance
    public var balances: [String:UInt64]

    /// the nonce or transaction count.
    public var nonce: UInt64

    /// the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public var pubKeyHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt64($0,false)! })
        nonce = try toUInt64(obj["nonce"],false)!
        pubKeyHash = try toString(obj["pubKeyHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["nonce"] = RPCObject(nonce)
        obj["pubKeyHash"] = RPCObject(pubKeyHash)
        return obj
    }
}

/// the state of all depositing-tx.
public struct ZksyncDepositing {
    /// the token-values.
    public var balances: [String:UInt64]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt64($0,false)! })
    }

}

/// the state after the rollup was verified in L1.
public struct ZksyncVerified {
    /// the token-balances.
    public var balances: [String:UInt64]

    /// the nonce or transaction count.
    public var nonce: UInt64

    /// the pubKeyHash set for the requested account or `0x0000...` if not set yet.
    public var pubKeyHash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        balances = try toObject(obj["balances"])!.mapValues({ try toUInt64($0,false)! })
        nonce = try toUInt64(obj["nonce"],false)!
        pubKeyHash = try toString(obj["pubKeyHash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["nonce"] = RPCObject(nonce)
        obj["pubKeyHash"] = RPCObject(pubKeyHash)
        return obj
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
    public var status: UInt64

    /// requested transactionHash
    public var transactionHash: String

    /// transactionIndex within the containing block.
    public var transactionIndex: UInt64

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        blockHash = try toString(obj["blockHash"],false)!
        contractAddress = try toString(obj["contractAddress"],false)!
        cumulativeGasUsed = try toUInt64(obj["cumulativeGasUsed"],false)!
        gasUsed = try toUInt64(obj["gasUsed"],false)!
        logs = try ZksyncLogs(obj["logs"],false)!
        logsBloom = try toString(obj["logsBloom"],false)!
        status = try toUInt64(obj["status"],false)!
        transactionHash = try toString(obj["transactionHash"],false)!
        transactionIndex = try toUInt64(obj["transactionIndex"],false)!
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
    public var logIndex: UInt64

    /// the reorg-status of the event.
    public var removed: Bool

    /// array of 32byte-topics of the indexed fields.
    public var topics: String

    /// requested transactionHash
    public var transactionHash: String

    /// transactionIndex within the containing block.
    public var transactionIndex: UInt64

    /// index of the event within the transaction.
    public var transactionLogIndex: UInt64

    /// mining-status
    public var type: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        address = try toString(obj["address"],false)!
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        blockHash = try toString(obj["blockHash"],false)!
        data = try toString(obj["data"],false)!
        logIndex = try toUInt64(obj["logIndex"],false)!
        removed = try toBool(obj["removed"],false)!
        topics = try toString(obj["topics"],false)!
        transactionHash = try toString(obj["transactionHash"],false)!
        transactionIndex = try toUInt64(obj["transactionIndex"],false)!
        transactionLogIndex = try toUInt64(obj["transactionLogIndex"],false)!
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
        obj["topics"] = RPCObject(topics)
        obj["transactionHash"] = RPCObject(transactionHash)
        obj["transactionIndex"] = RPCObject(transactionIndex)
        obj["transactionLogIndex"] = RPCObject(transactionLogIndex)
        obj["type"] = RPCObject(type)
        return obj
    }
}