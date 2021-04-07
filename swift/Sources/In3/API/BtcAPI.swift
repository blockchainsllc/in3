/// this is generated file don't edit it manually!

import Foundation

/// *Important: This feature is still experimental and not considered stable yet. In order to use it, you need to set the experimental-flag (-x on the comandline or `"experimental":true`!*
/// 
/// For bitcoin incubed follows the specification as defined in [https://bitcoincore.org/en/doc/0.18.0/](https://bitcoincore.org/en/doc/0.18.0/).
/// Internally the in3-server will add proofs as part of the responses. The proof data differs between the methods. You will read which proof data will be provided and how the data can be used to prove the result for each method. 
/// 
/// Proofs will add a special `in3`-section to the response containing a `proof`- object. This object will contain parts or all of the following properties:
/// 
/// *  **block**
/// *  **final**
/// *  **txIndex**
/// *  **merkleProof**
/// *  **cbtx**
/// *  **cbtxMerkleProof**
/// 
public class BtcAPI {
    internal var in3: In3

    /// initialiazes the Btc API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// returns a hex representation of the blockheader
    /// - Parameter hash : The block hash
    /// - Returns: the blockheader.     /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader    /// - verbose `1` or `true`: an object representing the blockheader.    /// 
    public func getblockheaderAsHex(hash: String) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getblockheader",params: RPCObject(hash), RPCObject(0),  convertWith: toString )
    }

    /// returns the blockheader
    /// - Parameter hash : The block hash
    /// - Returns: the blockheader.     /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader    /// - verbose `1` or `true`: an object representing the blockheader.    /// 
    public func getblockheader(hash: String) -> Future<Btcblockheader?> {
        return execAndConvertOptional(in3: in3, method: "getblockheader",params: RPCObject(hash), RPCObject(1),  convertWith: { try Btcblockheader($0,$1) } )
    }

    /// returns a hex representation of the block
    /// - Parameter hash : The block hash
    /// - Returns: the block.     /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader    /// - verbose `1` or `true`: an object representing the blockheader.    /// 
    public func getBlockAsHex(hash: String) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(0),  convertWith: toString )
    }

    /// returns the block with transactionhashes
    /// - Parameter hash : The block hash
    /// - Returns: the block.     /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader    /// - verbose `1` or `true`: an object representing the blockheader.    /// 
    public func getBlock(hash: String) -> Future<Btcblock?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(1),  convertWith: { try Btcblock($0,$1) } )
    }

    /// returns the block with full transactions
    /// - Parameter hash : The block hash
    /// - Returns: the block.     /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader    /// - verbose `1` or `true`: an object representing the blockheader.    /// 
    public func getBlockWithTx(hash: String) -> Future<BtcblockWithTx?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(2),  convertWith: { try BtcblockWithTx($0,$1) } )
    }

    /// returns a hex representation of the tx
    /// - Parameter txid : The transaction id
    /// - Parameter blockhash : The block in which to look for the transaction
    /// - Returns: - verbose `0` or `false`: a string that is serialized, hex-encoded data for `txid`    /// - verbose `1` or `false`: an object representing the transaction.            /// 
    public func getRawTransactionAsHex(txid: String, blockhash: String? = nil) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getrawtransaction",params: RPCObject(txid), RPCObject(0), blockhash == nil ? RPCObject.none : RPCObject(blockhash!), convertWith: toString )
    }

    /// returns the raw transaction
    /// - Parameter txid : The transaction id
    /// - Parameter blockhash : The block in which to look for the transaction
    /// - Returns: - verbose `0` or `false`: a string that is serialized, hex-encoded data for `txid`    /// - verbose `1` or `false`: an object representing the transaction.            /// 
    public func getRawTransaction(txid: String, blockhash: String? = nil) -> Future<Btctransaction?> {
        return execAndConvertOptional(in3: in3, method: "getrawtransaction",params: RPCObject(txid), RPCObject(1), blockhash == nil ? RPCObject.none : RPCObject(blockhash!), convertWith: { try Btctransaction($0,$1) } )
    }

    /// Returns the number of blocks in the longest blockchain.
    /// - Returns: the current blockheight
    public func getblockcount() -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "getblockcount", convertWith: toUInt64 )
    }

    /// Returns the proof-of-work difficulty as a multiple of the minimum difficulty.
    /// - Parameter blocknumber : Can be the number of a certain block to get its difficulty. To get the difficulty of the latest block use `latest`, `earliest`, `pending` or leave `params` empty (Hint: Latest block always means `actual latest block` minus `in3.finality`)
    /// - Returns: - `blocknumber` is a certain number: the difficulty of this block    /// - `blocknumber` is `latest`, `earliest`, `pending` or empty: the difficulty of the latest block (`actual latest block` minus `in3.finality`)    /// 
    public func getdifficulty(blocknumber: UInt64) -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "getdifficulty",params: RPCObject(blocknumber), convertWith: toUInt64 )
    }

    /// Whenever the client is not able to trust the changes of the target (which is the case if a block can't be found in the verified target cache *and* the value of the target changed more than the client's limit `max_diff`) he will call this method. It will return additional proof data to verify the changes of the target on the side of the client. This is not a standard Bitcoin rpc-method like the other ones, but more like an internal method.
    /// - Parameter target_dap : the number of the difficulty adjustment period (dap) we are looking for
    /// - Parameter verified_dap : the number of the closest already verified dap
    /// - Parameter max_diff : the maximum target difference between 2 verified daps
    /// - Parameter max_dap : the maximum amount of daps between 2 verified daps
    /// - Parameter limit : the maximum amount of daps to return (`0` = no limit) - this is important for embedded devices since returning all daps might be too much for limited memory
    /// - Returns: A path of daps from the `verified_dap` to the `target_dap` which fulfils the conditions of `max_diff`, `max_dap` and `limit`. Each dap of the path is a `dap`-object with corresponding proof data.
    public func proofTarget(target_dap: UInt64, verified_dap: UInt64, max_diff: UInt64? = 5, max_dap: UInt64? = 5, limit: UInt64? = 0) -> Future<[BtcProofTarget]> {
        return execAndConvert(in3: in3, method: "btc_proofTarget",params: RPCObject(target_dap), RPCObject(verified_dap),max_diff == nil ? RPCObject.none : RPCObject(max_diff!),max_dap == nil ? RPCObject.none : RPCObject(max_dap!),limit == nil ? RPCObject.none : RPCObject(limit!), convertWith: { try toArray($0,$1)!.map({ try BtcProofTarget($0,false)! }) } )
    }

    /// Returns the hash of the best (tip) block in the longest blockchain.
    /// - Returns: the hash of the best block
    public func getbestblockhash() -> Future<String> {
        return execAndConvert(in3: in3, method: "getbestblockhash", convertWith: toString )
    }


}
/// the blockheader. 
/// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
/// - verbose `1` or `true`: an object representing the blockheader.
/// 
public struct Btcblockheader {
    /// the block hash (same as provided)
    public var hash: String

    /// The number of confirmations, or -1 if the block is not on the main chain
    public var confirmations: Int

    /// The block height or index
    public var height: UInt64

    /// The block version
    public var version: UInt64

    /// The block version formatted in hexadecimal
    public var versionHex: String

    /// The merkle root ( 32 bytes )
    public var merkleroot: String

    /// The block time in seconds since epoch (Jan 1 1970 GMT)
    public var time: UInt64

    /// The median block time in seconds since epoch (Jan 1 1970 GMT)
    public var mediantime: UInt64

    /// The nonce
    public var nonce: UInt64

    /// The bits ( 4 bytes as hex) representing the target
    public var bits: String

    /// The difficulty
    public var difficulty: UInt64

    /// Expected number of hashes required to produce the current chain (in hex)
    public var chainwork: UInt64

    /// The number of transactions in the block.
    public var nTx: UInt64

    /// The hash of the previous block
    public var previousblockhash: String

    /// The hash of the next block
    public var nextblockhash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        hash = try toString(obj["hash"],false)!
        confirmations = try toInt(obj["confirmations"],false)!
        height = try toUInt64(obj["height"],false)!
        version = try toUInt64(obj["version"],false)!
        versionHex = try toString(obj["versionHex"],false)!
        merkleroot = try toString(obj["merkleroot"],false)!
        time = try toUInt64(obj["time"],false)!
        mediantime = try toUInt64(obj["mediantime"],false)!
        nonce = try toUInt64(obj["nonce"],false)!
        bits = try toString(obj["bits"],false)!
        difficulty = try toUInt64(obj["difficulty"],false)!
        chainwork = try toUInt64(obj["chainwork"],false)!
        nTx = try toUInt64(obj["nTx"],false)!
        previousblockhash = try toString(obj["previousblockhash"],false)!
        nextblockhash = try toString(obj["nextblockhash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["hash"] = RPCObject(hash)
        obj["confirmations"] = RPCObject(confirmations)
        obj["height"] = RPCObject(height)
        obj["version"] = RPCObject(version)
        obj["versionHex"] = RPCObject(versionHex)
        obj["merkleroot"] = RPCObject(merkleroot)
        obj["time"] = RPCObject(time)
        obj["mediantime"] = RPCObject(mediantime)
        obj["nonce"] = RPCObject(nonce)
        obj["bits"] = RPCObject(bits)
        obj["difficulty"] = RPCObject(difficulty)
        obj["chainwork"] = RPCObject(chainwork)
        obj["nTx"] = RPCObject(nTx)
        obj["previousblockhash"] = RPCObject(previousblockhash)
        obj["nextblockhash"] = RPCObject(nextblockhash)
        return obj
    }
}

/// the block. 
/// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
/// - verbose `1` or `true`: an object representing the blockheader.
/// 
public struct Btcblock {
    /// the block hash (same as provided)
    public var hash: String

    /// The number of confirmations, or -1 if the block is not on the main chain
    public var confirmations: Int

    /// The block height or index
    public var height: UInt64

    /// The block version
    public var version: UInt64

    /// The block version formatted in hexadecimal
    public var versionHex: String

    /// The merkle root ( 32 bytes )
    public var merkleroot: String

    /// The block time in seconds since epoch (Jan 1 1970 GMT)
    public var time: UInt64

    /// The median block time in seconds since epoch (Jan 1 1970 GMT)
    public var mediantime: UInt64

    /// The nonce
    public var nonce: UInt64

    /// The bits ( 4 bytes as hex) representing the target
    public var bits: String

    /// The difficulty
    public var difficulty: UInt64

    /// Expected number of hashes required to produce the current chain (in hex)
    public var chainwork: UInt64

    /// The number of transactions in the block.
    public var nTx: UInt64

    /// the array of transactions either as ids (verbose=1) or full transaction (verbose=2)
    public var tx: [String]

    /// The hash of the previous block
    public var previousblockhash: String

    /// The hash of the next block
    public var nextblockhash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        hash = try toString(obj["hash"],false)!
        confirmations = try toInt(obj["confirmations"],false)!
        height = try toUInt64(obj["height"],false)!
        version = try toUInt64(obj["version"],false)!
        versionHex = try toString(obj["versionHex"],false)!
        merkleroot = try toString(obj["merkleroot"],false)!
        time = try toUInt64(obj["time"],false)!
        mediantime = try toUInt64(obj["mediantime"],false)!
        nonce = try toUInt64(obj["nonce"],false)!
        bits = try toString(obj["bits"],false)!
        difficulty = try toUInt64(obj["difficulty"],false)!
        chainwork = try toUInt64(obj["chainwork"],false)!
        nTx = try toUInt64(obj["nTx"],false)!
        tx = try toArray(obj["tx"])!.map({ try toString($0,false)! })
        previousblockhash = try toString(obj["previousblockhash"],false)!
        nextblockhash = try toString(obj["nextblockhash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["hash"] = RPCObject(hash)
        obj["confirmations"] = RPCObject(confirmations)
        obj["height"] = RPCObject(height)
        obj["version"] = RPCObject(version)
        obj["versionHex"] = RPCObject(versionHex)
        obj["merkleroot"] = RPCObject(merkleroot)
        obj["time"] = RPCObject(time)
        obj["mediantime"] = RPCObject(mediantime)
        obj["nonce"] = RPCObject(nonce)
        obj["bits"] = RPCObject(bits)
        obj["difficulty"] = RPCObject(difficulty)
        obj["chainwork"] = RPCObject(chainwork)
        obj["nTx"] = RPCObject(nTx)
        obj["previousblockhash"] = RPCObject(previousblockhash)
        obj["nextblockhash"] = RPCObject(nextblockhash)
        return obj
    }
}

/// the block. 
/// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
/// - verbose `1` or `true`: an object representing the blockheader.
/// 
public struct BtcblockWithTx {
    /// the block hash (same as provided)
    public var hash: String

    /// The number of confirmations, or -1 if the block is not on the main chain
    public var confirmations: Int

    /// The block height or index
    public var height: UInt64

    /// The block version
    public var version: UInt64

    /// The block version formatted in hexadecimal
    public var versionHex: String

    /// The merkle root ( 32 bytes )
    public var merkleroot: String

    /// The block time in seconds since epoch (Jan 1 1970 GMT)
    public var time: UInt64

    /// The median block time in seconds since epoch (Jan 1 1970 GMT)
    public var mediantime: UInt64

    /// The nonce
    public var nonce: UInt64

    /// The bits ( 4 bytes as hex) representing the target
    public var bits: String

    /// The difficulty
    public var difficulty: UInt64

    /// Expected number of hashes required to produce the current chain (in hex)
    public var chainwork: UInt64

    /// The number of transactions in the block.
    public var nTx: UInt64

    /// the array of transactions either as ids (verbose=1) or full transaction (verbose=2)
    public var tx: [Btctransaction]

    /// The hash of the previous block
    public var previousblockhash: String

    /// The hash of the next block
    public var nextblockhash: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        hash = try toString(obj["hash"],false)!
        confirmations = try toInt(obj["confirmations"],false)!
        height = try toUInt64(obj["height"],false)!
        version = try toUInt64(obj["version"],false)!
        versionHex = try toString(obj["versionHex"],false)!
        merkleroot = try toString(obj["merkleroot"],false)!
        time = try toUInt64(obj["time"],false)!
        mediantime = try toUInt64(obj["mediantime"],false)!
        nonce = try toUInt64(obj["nonce"],false)!
        bits = try toString(obj["bits"],false)!
        difficulty = try toUInt64(obj["difficulty"],false)!
        chainwork = try toUInt64(obj["chainwork"],false)!
        nTx = try toUInt64(obj["nTx"],false)!
        tx = try toArray(obj["tx"])!.map({ try Btctransaction($0,false)! })
        previousblockhash = try toString(obj["previousblockhash"],false)!
        nextblockhash = try toString(obj["nextblockhash"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["hash"] = RPCObject(hash)
        obj["confirmations"] = RPCObject(confirmations)
        obj["height"] = RPCObject(height)
        obj["version"] = RPCObject(version)
        obj["versionHex"] = RPCObject(versionHex)
        obj["merkleroot"] = RPCObject(merkleroot)
        obj["time"] = RPCObject(time)
        obj["mediantime"] = RPCObject(mediantime)
        obj["nonce"] = RPCObject(nonce)
        obj["bits"] = RPCObject(bits)
        obj["difficulty"] = RPCObject(difficulty)
        obj["chainwork"] = RPCObject(chainwork)
        obj["nTx"] = RPCObject(nTx)
        obj["previousblockhash"] = RPCObject(previousblockhash)
        obj["nextblockhash"] = RPCObject(nextblockhash)
        return obj
    }
}

/// the array of transactions either as ids (verbose=1) or full transaction (verbose=2)
public struct Btctransaction {
    /// txid
    public var txid: String

    /// Whether specified block is in the active chain or not (only present with explicit "blockhash" argument)
    public var in_active_chain: Bool

    /// The serialized, hex-encoded data for `txid`
    public var hex: String

    /// The transaction hash (differs from txid for witness transactions)
    public var hash: String

    /// The serialized transaction size
    public var size: UInt64

    /// The virtual transaction size (differs from size for witness transactions)
    public var vsize: UInt64

    /// The transaction's weight (between `vsize`\*4-3 and `vsize`\*4)
    public var weight: UInt64

    /// The version
    public var version: UInt64

    /// The lock time
    public var locktime: UInt64

    /// array of json objects of incoming txs to be used
    public var vin: [BtcVin]

    /// array of json objects describing the tx outputs
    public var vout: [BtcVout]

    /// the block hash
    public var blockhash: String

    /// The confirmations
    public var confirmations: Int

    /// The block time in seconds since epoch (Jan 1 1970 GMT)
    public var blocktime: UInt64

    /// Same as "blocktime"
    public var time: UInt64

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        txid = try toString(obj["txid"],false)!
        in_active_chain = try toBool(obj["in_active_chain"],false)!
        hex = try toString(obj["hex"],false)!
        hash = try toString(obj["hash"],false)!
        size = try toUInt64(obj["size"],false)!
        vsize = try toUInt64(obj["vsize"],false)!
        weight = try toUInt64(obj["weight"],false)!
        version = try toUInt64(obj["version"],false)!
        locktime = try toUInt64(obj["locktime"],false)!
        vin = try toArray(obj["vin"])!.map({ try BtcVin($0,false)! })
        vout = try toArray(obj["vout"])!.map({ try BtcVout($0,false)! })
        blockhash = try toString(obj["blockhash"],false)!
        confirmations = try toInt(obj["confirmations"],false)!
        blocktime = try toUInt64(obj["blocktime"],false)!
        time = try toUInt64(obj["time"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["txid"] = RPCObject(txid)
        obj["in_active_chain"] = RPCObject(in_active_chain)
        obj["hex"] = RPCObject(hex)
        obj["hash"] = RPCObject(hash)
        obj["size"] = RPCObject(size)
        obj["vsize"] = RPCObject(vsize)
        obj["weight"] = RPCObject(weight)
        obj["version"] = RPCObject(version)
        obj["locktime"] = RPCObject(locktime)
        obj["blockhash"] = RPCObject(blockhash)
        obj["confirmations"] = RPCObject(confirmations)
        obj["blocktime"] = RPCObject(blocktime)
        obj["time"] = RPCObject(time)
        return obj
    }
}

/// array of json objects of incoming txs to be used
public struct BtcVin {
    /// the transaction id
    public var txid: String

    /// the index of the transaction out to be used
    public var vout: UInt64

    /// the script
    public var scriptSig: BtcScriptSig

    /// The script sequence number
    public var sequence: UInt64

    /// hex-encoded witness data (if any)
    public var txinwitness: [String]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        txid = try toString(obj["txid"],false)!
        vout = try toUInt64(obj["vout"],false)!
        scriptSig = try BtcScriptSig(obj["scriptSig"],false)!
        sequence = try toUInt64(obj["sequence"],false)!
        txinwitness = try toArray(obj["txinwitness"])!.map({ try toString($0,false)! })
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["txid"] = RPCObject(txid)
        obj["vout"] = RPCObject(vout)
        obj["sequence"] = RPCObject(sequence)
        return obj
    }
}

/// the script
public struct BtcScriptSig {
    /// the asm-codes
    public var asm: String

    /// hex representation
    public var hex: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        asm = try toString(obj["asm"],false)!
        hex = try toString(obj["hex"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["asm"] = RPCObject(asm)
        obj["hex"] = RPCObject(hex)
        return obj
    }
}

/// array of json objects describing the tx outputs
public struct BtcVout {
    /// The Value in BTC
    public var value: UInt64

    /// the index
    public var n: UInt64

    /// the script pubkey
    public var scriptPubKey: BtcScriptPubKey

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        value = try toUInt64(obj["value"],false)!
        n = try toUInt64(obj["n"],false)!
        scriptPubKey = try BtcScriptPubKey(obj["scriptPubKey"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["value"] = RPCObject(value)
        obj["n"] = RPCObject(n)
        return obj
    }
}

/// the script pubkey
public struct BtcScriptPubKey {
    /// asm
    public var asm: String

    /// hex representation of the script
    public var hex: String

    /// the required signatures
    public var reqSigs: UInt64

    /// The type, eg 'pubkeyhash'
    public var type: String

    /// Array of address(each representing a bitcoin adress)
    public var addresses: [String]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        asm = try toString(obj["asm"],false)!
        hex = try toString(obj["hex"],false)!
        reqSigs = try toUInt64(obj["reqSigs"],false)!
        type = try toString(obj["type"],false)!
        addresses = try toArray(obj["addresses"])!.map({ try toString($0,false)! })
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["asm"] = RPCObject(asm)
        obj["hex"] = RPCObject(hex)
        obj["reqSigs"] = RPCObject(reqSigs)
        obj["type"] = RPCObject(type)
        return obj
    }
}

/// A path of daps from the `verified_dap` to the `target_dap` which fulfils the conditions of `max_diff`, `max_dap` and `limit`. Each dap of the path is a `dap`-object with corresponding proof data.
public struct BtcProofTarget {
    /// the difficulty adjustement period
    public var dap: UInt64

    /// the first blockheader
    public var block: String

    /// the finality header
    public var final: String

    /// the coinbase transaction as hex
    public var cbtx: String

    /// the coinbasetx merkle proof
    public var cbtxMerkleProof: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        dap = try toUInt64(obj["dap"],false)!
        block = try toString(obj["block"],false)!
        final = try toString(obj["final"],false)!
        cbtx = try toString(obj["cbtx"],false)!
        cbtxMerkleProof = try toString(obj["cbtxMerkleProof"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["dap"] = RPCObject(dap)
        obj["block"] = RPCObject(block)
        obj["final"] = RPCObject(final)
        obj["cbtx"] = RPCObject(cbtx)
        obj["cbtxMerkleProof"] = RPCObject(cbtxMerkleProof)
        return obj
    }
}