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
    /// - Returns: the blockheader. 
    /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
    /// - verbose `1` or `true`: an object representing the blockheader.
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getblockheaderAsHex(hash: "000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 3045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745e4...f3255d
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getblockheaderAsHex(hash: String) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getblockheader",params: RPCObject(hash), RPCObject(0),  convertWith: toString )
    }

    /// returns the blockheader
    /// - Parameter hash : The block hash
    /// - Returns: the blockheader. 
    /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
    /// - verbose `1` or `true`: an object representing the blockheader.
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getblockheader(hash: "000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          hash: 000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220
    /// //          confirmations: 8268
    /// //          height: 624958
    /// //          version: 536928256
    /// //          versionHex: 2000
    /// //          merkleroot: d786a334ea8c65f39272d5b9be505ac3170f3904842bd52525538a9377b359cb
    /// //          time: 1586333924
    /// //          mediantime: 1586332639
    /// //          nonce: 1985217615
    /// //          bits: 17143b41
    /// //          difficulty: 13912524048945.91
    /// //          chainwork: 00000000000000000000000000000000000000000e4c88b66c5ee78deff0d494
    /// //          nTx: 33
    /// //          previousblockhash: 00000000000000000013cba040837778744ce66961cfcf2e7c34bb3d194c7f49
    /// //          nextblockhash: 0000000000000000000c799dc0e36302db7fbb471711f140dc308508ef19e343
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getblockheader(hash: String) -> Future<Btcblockheader?> {
        return execAndConvertOptional(in3: in3, method: "getblockheader",params: RPCObject(hash), RPCObject(1),  convertWith: { try Btcblockheader($0,$1) } )
    }

    /// returns a hex representation of the block
    /// - Parameter hash : The block hash
    /// - Returns: the block. 
    /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
    /// - verbose `1` or `true`: an object representing the blockheader.
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getBlockAsHex(hash: "000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 3045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745e4...f3255d
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlockAsHex(hash: String) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(0),  convertWith: toString )
    }

    /// returns the block with transactionhashes
    /// - Parameter hash : The block hash
    /// - Returns: the block. 
    /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
    /// - verbose `1` or `true`: an object representing the blockheader.
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getBlock(hash: "000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          hash: 000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220
    /// //          confirmations: 8268
    /// //          height: 624958
    /// //          version: 536928256
    /// //          versionHex: 2000
    /// //          merkleroot: d786a334ea8c65f39272d5b9be505ac3170f3904842bd52525538a9377b359cb
    /// //          time: 1586333924
    /// //          mediantime: 1586332639
    /// //          nonce: 1985217615
    /// //          bits: 17143b41
    /// //          difficulty: 13912524048945.91
    /// //          chainwork: 00000000000000000000000000000000000000000e4c88b66c5ee78deff0d494
    /// //          tx:
    /// //            - d79ffc80e07fe9e0083319600c59d47afe69995b1357be6e5dba035675780290
    /// //            - ...
    /// //            - 6456819bfa019ba30788620153ea9a361083cb888b3662e2ff39c0f7adf16919
    /// //          nTx: 33
    /// //          previousblockhash: 00000000000000000013cba040837778744ce66961cfcf2e7c34bb3d194c7f49
    /// //          nextblockhash: 0000000000000000000c799dc0e36302db7fbb471711f140dc308508ef19e343
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlock(hash: String) -> Future<Btcblock?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(1),  convertWith: { try Btcblock($0,$1) } )
    }

    /// returns the block with full transactions
    /// - Parameter hash : The block hash
    /// - Returns: the block. 
    /// - verbose `0` or `false`: a hex string with 80 bytes representing the blockheader
    /// - verbose `1` or `true`: an object representing the blockheader.
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getBlockWithTx(hash: "000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          hash: 000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220
    /// //          confirmations: 8268
    /// //          height: 624958
    /// //          version: 536928256
    /// //          versionHex: 2000
    /// //          merkleroot: d786a334ea8c65f39272d5b9be505ac3170f3904842bd52525538a9377b359cb
    /// //          time: 1586333924
    /// //          mediantime: 1586332639
    /// //          nonce: 1985217615
    /// //          bits: 17143b41
    /// //          difficulty: 13912524048945.91
    /// //          chainwork: 00000000000000000000000000000000000000000e4c88b66c5ee78deff0d494
    /// //          tx:
    /// //            - d79ffc80e07fe9e0083319600c59d47afe69995b1357be6e5dba035675780290
    /// //            - ...
    /// //            - 6456819bfa019ba30788620153ea9a361083cb888b3662e2ff39c0f7adf16919
    /// //          nTx: 33
    /// //          previousblockhash: 00000000000000000013cba040837778744ce66961cfcf2e7c34bb3d194c7f49
    /// //          nextblockhash: 0000000000000000000c799dc0e36302db7fbb471711f140dc308508ef19e343
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlockWithTx(hash: String) -> Future<BtcblockWithTx?> {
        return execAndConvertOptional(in3: in3, method: "getblock",params: RPCObject(hash), RPCObject(2),  convertWith: { try BtcblockWithTx($0,$1) } )
    }

    /// returns a hex representation of the tx
    /// - Parameter txid : The transaction id
    /// - Parameter blockhash : The block in which to look for the transaction
    /// - Returns: - verbose `0` or `false`: a string that is serialized, hex-encoded data for `txid`
    /// - verbose `1` or `false`: an object representing the transaction.        
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getRawTransactionAsHex(txid: "f3c06e17b04ef748ce6604ad68e5b9f68ca96914b57c2118a1bb9a09a194ddaf", verbosity: true) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 3045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745e4...f3255d
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getRawTransactionAsHex(txid: String, blockhash: String? = nil) -> Future<String?> {
        return execAndConvertOptional(in3: in3, method: "getrawtransaction",params: RPCObject(txid), RPCObject(0), blockhash == nil ? RPCObject.none : RPCObject(blockhash!), convertWith: toString )
    }

    /// returns the raw transaction
    /// - Parameter txid : The transaction id
    /// - Parameter blockhash : The block in which to look for the transaction
    /// - Returns: - verbose `0` or `false`: a string that is serialized, hex-encoded data for `txid`
    /// - verbose `1` or `false`: an object representing the transaction.        
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getRawTransaction(txid: "f3c06e17b04ef748ce6604ad68e5b9f68ca96914b57c2118a1bb9a09a194ddaf", verbosity: true) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          in_active_chain: true
    /// //          txid: f3c06e17b04ef748ce6604ad68e5b9f68ca96914b57c2118a1bb9a09a194ddaf
    /// //          hash: f3c06e17b04ef748ce6604ad68e5b9f68ca96914b57c2118a1bb9a09a194ddaf
    /// //          version: 1
    /// //          size: 518
    /// //          vsize: 518
    /// //          weight: 2072
    /// //          locktime: 0
    /// //          vin:
    /// //            - txid: 0a74f6e5f99bc69af80da9f0d9878ea6afbfb5fbb2d43f1ff899bcdd641a098c
    /// //              vout: 0
    /// //              scriptSig:
    /// //                asm: 30440220481f2b3a49b202e26c73ac1b7bce022e4a74aff08473228cc...254874
    /// //                hex: 4730440220481f2b3a49b202e26c73ac1b7bce022e4a74aff08473228...254874
    /// //              sequence: 4294967295
    /// //            - txid: 869c5e82d4dfc3139c8a153d2ee126e30a467cf791718e6ea64120e5b19e5044
    /// //              vout: 0
    /// //              scriptSig:
    /// //                asm: 3045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745e4...f3255d
    /// //                hex: 483045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745...f3255d
    /// //              sequence: 4294967295
    /// //            - txid: 8a03d29a1b8ae408c94a2ae15bef8329bc3d6b04c063d36b2e8c997273fa8eff
    /// //              vout: 1
    /// //              scriptSig:
    /// //                asm: 304402200bf7c5c7caec478bf6d7e9c5127c71505034302056d1284...0045da
    /// //                hex: 47304402200bf7c5c7caec478bf6d7e9c5127c71505034302056d12...0045da
    /// //              sequence: 4294967295
    /// //          vout:
    /// //            - value: 0.00017571
    /// //              n: 0
    /// //              scriptPubKey:
    /// //                asm: OP_DUP OP_HASH160 53196749b85367db9443ef9a5aec25cf0bdceedf OP_EQUALVERIFY
    /// //                  OP_CHECKSIG
    /// //                hex: 76a91453196749b85367db9443ef9a5aec25cf0bdceedf88ac
    /// //                reqSigs: 1
    /// //                type: pubkeyhash
    /// //                addresses:
    /// //                  - 18aPWzBTq1nzs9o86oC9m3BQbxZWmV82UU
    /// //            - value: 0.00915732
    /// //              n: 1
    /// //              scriptPubKey:
    /// //                asm: OP_HASH160 8bb2b4b848d0b6336cc64ea57ae989630f447cba OP_EQUAL
    /// //                hex: a9148bb2b4b848d0b6336cc64ea57ae989630f447cba87
    /// //                reqSigs: 1
    /// //                type: scripthash
    /// //                addresses:
    /// //                  - 3ERfvuzAYPPpACivh1JnwYbBdrAjupTzbw
    /// //          hex: 01000000038c091a64ddbc99f81f3fd4b2fbb5bfafa68e8...000000
    /// //          blockhash: 000000000000000000103b2395f6cd94221b10d02eb9be5850303c0534307220
    /// //          confirmations: 15307
    /// //          time: 1586333924
    /// //          blocktime: 1586333924
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getRawTransaction(txid: String, blockhash: String? = nil) -> Future<Btctransaction?> {
        return execAndConvertOptional(in3: in3, method: "getrawtransaction",params: RPCObject(txid), RPCObject(1), blockhash == nil ? RPCObject.none : RPCObject(blockhash!), convertWith: { try Btctransaction($0,$1) } )
    }

    /// Returns the number of blocks in the longest blockchain.
    /// - Returns: the current blockheight
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getblockcount() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 640387
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getblockcount() -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "getblockcount", convertWith: toUInt64 )
    }

    /// Returns the proof-of-work difficulty as a multiple of the minimum difficulty.
    /// - Parameter blocknumber : Can be the number of a certain block to get its difficulty. To get the difficulty of the latest block use `latest`, `earliest`, `pending` or leave `params` empty (Hint: Latest block always means `actual latest block` minus `in3.finality`)
    /// - Returns: - `blocknumber` is a certain number: the difficulty of this block
    /// - `blocknumber` is `latest`, `earliest`, `pending` or empty: the difficulty of the latest block (`actual latest block` minus `in3.finality`)
    /// 
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getdifficulty(blocknumber: 631910) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 15138043247082.88
    ///      }
    /// }
    /// 
    /// ```
    /// 
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
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).proofTarget(target_dap: 230, verified_dap: 200, max_diff: 5, max_dap: 5, limit: 15) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          - dap: 205
    /// //            block: 0x04000000e62ef28cb9793f4f9cd2a67a58c1e7b593129b9b...0ab284
    /// //            final: 0x04000000cc69b68b702321adf4b0c485fdb1f3d6c1ddd140...090a5b
    /// //            cbtx: 0x01000000...1485ce370573be63d7cc1b9efbad3489eb57c8...000000
    /// //            cbtxMerkleProof: 0xc72dffc1cb4cbeab960d0d2bdb80012acf7f9c...affcf4
    /// //          - dap: 210
    /// //            block: 0x0000003021622c26a4e62cafa8e434c7e083f540bccc8392...b374ce
    /// //            final: 0x00000020858f8e5124cd516f4d5e6a078f7083c12c48e8cd...308c3d
    /// //            cbtx: 0x01000000...c075061b4b6e434d696e657242332d50314861...000000
    /// //            cbtxMerkleProof: 0xf2885d0bac15fca7e1644c1162899ecd43d52b...93761d
    /// //          - dap: 215
    /// //            block: 0x000000202509b3b8e4f98290c7c9551d180eb2a463f0b978...f97b64
    /// //            final: 0x0000002014c7c0ed7c33c59259b7b508bebfe3974e1c99a5...eb554e
    /// //            cbtx: 0x01000000...90133cf94b1b1c40fae077a7833c0fe0ccc474...000000
    /// //            cbtxMerkleProof: 0x628c8d961adb157f800be7cfb03ffa1b53d3ad...ca5a61
    /// //          - dap: 220
    /// //            block: 0x00000020ff45c783d09706e359dcc76083e15e51839e4ed5...ddfe0e
    /// //            final: 0x0000002039d2f8a1230dd0bee50034e8c63951ab812c0b89...5670c5
    /// //            cbtx: 0x01000000...b98e79fb3e4b88aefbc8ce59e82e99293e5b08...000000
    /// //            cbtxMerkleProof: 0x16adb7aeec2cf254db0bab0f4a5083fb0e0a3f...63a4f4
    /// //          - dap: 225
    /// //            block: 0x02000020170fad0b6b1ccbdc4401d7b1c8ee868c6977d6ce...1e7f8f
    /// //            final: 0x0400000092945abbd7b9f0d407fcccbf418e4fc20570040c...a9b240
    /// //            cbtx: 0x01000000...cf6e8f930acb8f38b588d76cd8c3da3258d5a7...000000
    /// //            cbtxMerkleProof: 0x25575bcaf3e11970ccf835e88d6f97bedd6b85...bfdf46
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func proofTarget(target_dap: UInt64, verified_dap: UInt64, max_diff: UInt64? = 5, max_dap: UInt64? = 5, limit: UInt64? = 0) -> Future<[BtcProofTarget]> {
        return execAndConvert(in3: in3, method: "btc_proofTarget",params: RPCObject(target_dap), RPCObject(verified_dap),max_diff == nil ? RPCObject.none : RPCObject(max_diff!),max_dap == nil ? RPCObject.none : RPCObject(max_dap!),limit == nil ? RPCObject.none : RPCObject(limit!), convertWith: { try toArray($0,$1)!.map({ try BtcProofTarget($0,false)! }) } )
    }

    /// Returns the hash of the best (tip) block in the longest blockchain.
    /// - Returns: the hash of the best block
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// BtcAPI(in3).getbestblockhash() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 000000000000000000039cbb4e842de0de9651852122b117d7ae6d7ac4fc1df6
    ///      }
    /// }
    /// 
    /// ```
    /// 
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