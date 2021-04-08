/// this is generated file don't edit it manually!

import Foundation

/// Standard JSON-RPC calls as described in https://eth.wiki/json-rpc/API.
/// 
/// Whenever a request is made for a response with `verification`: `proof`, the node must provide the proof needed to validate the response result. The proof itself depends on the chain.
/// 
/// For ethereum, all proofs are based on the correct block hash. That's why verification differentiates between [Verifying the blockhash](poa.html) (which depends on the user consensus) the actual result data.
/// 
/// There is another reason why the BlockHash is so important. This is the only value you are able to access from within a SmartContract, because the evm supports a OpCode (`BLOCKHASH`), which allows you to read the last 256 blockhashes, which gives us the chance to verify even the blockhash onchain.
/// 
/// Depending on the method, different proofs are needed, which are described in this document.
/// 
/// Proofs will add a special in3-section to the response containing a `proof`- object. Each `in3`-section of the response containing proofs has a property with a proof-object with the following properties:
/// 
/// *  **type** `string` (required)  - The type of the proof.   
/// Must be one of the these values : `'transactionProof`', `'receiptProof`', `'blockProof`', `'accountProof`', `'callProof`', `'logProof`'
/// *  **block** `string` - The serialized blockheader as hex, required in most proofs. 
/// *  **finalityBlocks** `array` - The serialized following blockheaders as hex, required in case of finality asked (only relevant for PoA-chains). The server must deliver enough blockheaders to cover more then 50% of the validators. In order to verify them, they must be linkable (with the parentHash).    
/// *  **transactions** `array` - The list of raw transactions of the block if needed to create a merkle trie for the transactions. 
/// *  **uncles** `array` - The list of uncle-headers of the block. This will only be set if full verification is required in order to create a merkle tree for the uncles and so prove the uncle_hash.   
/// *  **merkleProof** `string[]` - The serialized merkle-nodes beginning with the root-node (depending on the content to prove).
/// *  **merkleProofPrev** `string[]` - The serialized merkle-nodes beginning with the root-node of the previous entry (only for full proof of receipts).   
/// *  **txProof** `string[]` - The serialized merkle-nodes beginning with the root-node in order to proof the transactionIndex (only needed for transaction receipts).
/// *  **logProof** [LogProof](#logproof) - The Log Proof in case of a `eth_getLogs`-request.   
/// *  **accounts** `object` - A map of addresses and their AccountProof.   
/// *  **txIndex** `integer` - The transactionIndex within the block (for transaactions and receipts).   
/// *  **signatures** `Signature[]` - Requested signatures.   
/// 
public class EthAPI {
    internal var in3: In3

    /// initialiazes the Eth API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
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
    /// let result = try EthAPI(in3).keccak(data: "0x1234567890")
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
    /// let result = try EthAPI(in3).sha3(data: "0x1234567890")
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
    /// let result = try EthAPI(in3).sha256(data: "0x1234567890")
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
    /// let result = try EthAPI(in3).createKey()
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
    /// EthAPI(in3).sign(account: "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83", message: "0xdeadbeaf") .observe(using: {
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
    /// EthAPI(in3).signTransaction(tx: EthTransaction(data: "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675", from: "0xb60e8dd61c5d32be8058bb8eb970870f07233155", gas: "0x76c0", gasPrice: "0x9184e72a000", to: "0xd46e8dd67c5d32be8058bb8eb970870f07244567", value: "0x9184e72a")) .observe(using: {
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
    public func signTransaction(tx: EthTransaction) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_signTransaction", params:RPCObject( tx.toRPCDict()), convertWith: toString )
    }

    /// returns the number of the most recent block.
    /// 
    /// See [eth_blockNumber](https://eth.wiki/json-rpc/API#eth_blockNumber) for spec.
    /// 
    /// No proof returned, since there is none, but the client should verify the result by comparing it to the current blocks returned from others. 
    /// With the `blockTime` from the chainspec, including a tolerance, the current blocknumber may be checked if in the proposed range.
    /// 
    /// - Returns: the highest known blocknumber
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).blockNumber() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0xb8a2a5"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func blockNumber() -> Future<UInt64> {
        return execAndConvert(in3: in3, method: "eth_blockNumber", convertWith: toUInt64 )
    }

    /// returns the given Block by number with transactionHashes. if no blocknumber is specified the latest block will be returned.
    /// - Parameter blockNumber : the blockNumber or one of `latest`, `earliest`or `pending`
    /// - Returns: the blockdata, or in case the block with that number does not exist, `null` will be returned.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getBlock() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          author: "0x0000000000000000000000000000000000000000"
    /// //          difficulty: "0x2"
    /// //          extraData: 0x696e667572612d696f0000000000000...31570f1e500
    /// //          gasLimit: "0x7a1200"
    /// //          gasUsed: "0x20e145"
    /// //          hash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585"
    /// //          logsBloom: 0x000008000000000000...00400100000000080
    /// //          miner: "0x0000000000000000000000000000000000000000"
    /// //          number: "0x449956"
    /// //          parentHash: "0x2c2a4fcd11aa9aea6b9767651a10e7dbd2bcddbdaba703c74458ad6faf7c2694"
    /// //          receiptsRoot: "0x0240b90272b5600bef7e25d0894868f85125174c2f387ef3236fc9ed9bfb3eff"
    /// //          sealFields:
    /// //            - "0xa00000000000000000000000000000000000000000000000000000000000000000"
    /// //            - "0x880000000000000000"
    /// //          sha3Uncles: "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
    /// //          size: "0x74b"
    /// //          stateRoot: "0xf44699575afd2668060be5ba77e66e1e80edb77ad1b5070969ddfa63da6a4910"
    /// //          timestamp: "0x605aec86"
    /// //          totalDifficulty: "0x6564de"
    /// //          transactions:
    /// //            - "0xcb7edfdb3229c9beeb418ab1ef1a3c9210ecfb22f0157791c3287085d798da58"
    /// //            - "0x0fb803696521ba109c40b3eecb773c93dc6ee891172af0f620c8d44c05198641"
    /// //            - "0x3ef6725cab4470889c3c7d53609a5d4b263701f5891aa98c9ed48b73b6b2fb75"
    /// //            - "0x4010c4c112514756dcdcf14f91117503826dcbe15b03a1636c07aa713da24b8d"
    /// //            - "0xd9c14daa5e2e9cc955534865365ef6bde3045c70e3a984a74c298606c4d67bb5"
    /// //            - "0xfa2326237ba5dcca2127241562be16b68c48fed93d29add8d62f79a00518c2d8"
    /// //          transactionsRoot: "0xddbbd7bf723abdfe885539406540671c2c0eb97684972175ad199258c75416cc"
    /// //          uncles: []
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlock(blockNumber: UInt64? = nil) -> Future<EthBlockdataWithTxHashes?> {
        return execAndConvertOptional(in3: in3, method: "eth_getBlockByNumber", params:blockNumber == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", blockNumber!) ),  RPCObject(false), convertWith: { try EthBlockdataWithTxHashes($0,$1) } )
    }

    /// returns the given Block by number with full transaction data. if no blocknumber is specified the latest block will be returned.
    /// - Parameter blockNumber : the blockNumber or one of `latest`, `earliest`or `pending`
    /// - Returns: the blockdata, or in case the block with that number does not exist, `null` will be returned.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getBlockWithTx() .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          author: "0x0000000000000000000000000000000000000000"
    /// //          difficulty: "0x2"
    /// //          extraData: 0x696e667572612d696f0000000000000...31570f1e500
    /// //          gasLimit: "0x7a1200"
    /// //          gasUsed: "0x20e145"
    /// //          hash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585"
    /// //          logsBloom: 0x000008000000000000...00400100000000080
    /// //          miner: "0x0000000000000000000000000000000000000000"
    /// //          number: "0x449956"
    /// //          parentHash: "0x2c2a4fcd11aa9aea6b9767651a10e7dbd2bcddbdaba703c74458ad6faf7c2694"
    /// //          receiptsRoot: "0x0240b90272b5600bef7e25d0894868f85125174c2f387ef3236fc9ed9bfb3eff"
    /// //          sealFields:
    /// //            - "0xa00000000000000000000000000000000000000000000000000000000000000000"
    /// //            - "0x880000000000000000"
    /// //          sha3Uncles: "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
    /// //          size: "0x74b"
    /// //          stateRoot: "0xf44699575afd2668060be5ba77e66e1e80edb77ad1b5070969ddfa63da6a4910"
    /// //          timestamp: "0x605aec86"
    /// //          totalDifficulty: "0x6564de"
    /// //          transactions:
    /// //            - "0xcb7edfdb3229c9beeb418ab1ef1a3c9210ecfb22f0157791c3287085d798da58"
    /// //            - "0x0fb803696521ba109c40b3eecb773c93dc6ee891172af0f620c8d44c05198641"
    /// //            - "0x3ef6725cab4470889c3c7d53609a5d4b263701f5891aa98c9ed48b73b6b2fb75"
    /// //            - "0x4010c4c112514756dcdcf14f91117503826dcbe15b03a1636c07aa713da24b8d"
    /// //            - "0xd9c14daa5e2e9cc955534865365ef6bde3045c70e3a984a74c298606c4d67bb5"
    /// //            - "0xfa2326237ba5dcca2127241562be16b68c48fed93d29add8d62f79a00518c2d8"
    /// //          transactionsRoot: "0xddbbd7bf723abdfe885539406540671c2c0eb97684972175ad199258c75416cc"
    /// //          uncles: []
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlockWithTx(blockNumber: UInt64? = nil) -> Future<EthBlockdata?> {
        return execAndConvertOptional(in3: in3, method: "eth_getBlockByNumber", params:blockNumber == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", blockNumber!) ),  RPCObject(false), convertWith: { try EthBlockdata($0,$1) } )
    }

    /// returns the given Block by hash with transactionHashes
    /// - Parameter blockHash : the blockHash of the block
    /// - Returns: the blockdata, or in case the block with that number does not exist, `null` will be returned.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getBlockByHash(blockHash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          author: "0x0000000000000000000000000000000000000000"
    /// //          difficulty: "0x2"
    /// //          extraData: 0x696e667572612d696f0000000000000...31570f1e500
    /// //          gasLimit: "0x7a1200"
    /// //          gasUsed: "0x20e145"
    /// //          hash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585"
    /// //          logsBloom: 0x000008000000000000...00400100000000080
    /// //          miner: "0x0000000000000000000000000000000000000000"
    /// //          number: "0x449956"
    /// //          parentHash: "0x2c2a4fcd11aa9aea6b9767651a10e7dbd2bcddbdaba703c74458ad6faf7c2694"
    /// //          receiptsRoot: "0x0240b90272b5600bef7e25d0894868f85125174c2f387ef3236fc9ed9bfb3eff"
    /// //          sealFields:
    /// //            - "0xa00000000000000000000000000000000000000000000000000000000000000000"
    /// //            - "0x880000000000000000"
    /// //          sha3Uncles: "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
    /// //          size: "0x74b"
    /// //          stateRoot: "0xf44699575afd2668060be5ba77e66e1e80edb77ad1b5070969ddfa63da6a4910"
    /// //          timestamp: "0x605aec86"
    /// //          totalDifficulty: "0x6564de"
    /// //          transactions:
    /// //            - "0xcb7edfdb3229c9beeb418ab1ef1a3c9210ecfb22f0157791c3287085d798da58"
    /// //            - "0x0fb803696521ba109c40b3eecb773c93dc6ee891172af0f620c8d44c05198641"
    /// //            - "0x3ef6725cab4470889c3c7d53609a5d4b263701f5891aa98c9ed48b73b6b2fb75"
    /// //            - "0x4010c4c112514756dcdcf14f91117503826dcbe15b03a1636c07aa713da24b8d"
    /// //            - "0xd9c14daa5e2e9cc955534865365ef6bde3045c70e3a984a74c298606c4d67bb5"
    /// //            - "0xfa2326237ba5dcca2127241562be16b68c48fed93d29add8d62f79a00518c2d8"
    /// //          transactionsRoot: "0xddbbd7bf723abdfe885539406540671c2c0eb97684972175ad199258c75416cc"
    /// //          uncles: []
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlockByHash(blockHash: String) -> Future<EthBlockdataWithTxHashes?> {
        return execAndConvertOptional(in3: in3, method: "eth_getBlockByHash", params:RPCObject( blockHash),  RPCObject(false), convertWith: { try EthBlockdataWithTxHashes($0,$1) } )
    }

    /// returns the given Block by hash with full transaction data
    /// - Parameter blockHash : the blockHash of the block
    /// - Returns: the blockdata, or in case the block with that number does not exist, `null` will be returned.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getBlockByHashWithTx(blockHash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          author: "0x0000000000000000000000000000000000000000"
    /// //          difficulty: "0x2"
    /// //          extraData: 0x696e667572612d696f0000000000000...31570f1e500
    /// //          gasLimit: "0x7a1200"
    /// //          gasUsed: "0x20e145"
    /// //          hash: "0x2baa54adcd8a105cdedfd9c6635d48d07b8f0e805af0a5853190c179e5a18585"
    /// //          logsBloom: 0x000008000000000000...00400100000000080
    /// //          miner: "0x0000000000000000000000000000000000000000"
    /// //          number: "0x449956"
    /// //          parentHash: "0x2c2a4fcd11aa9aea6b9767651a10e7dbd2bcddbdaba703c74458ad6faf7c2694"
    /// //          receiptsRoot: "0x0240b90272b5600bef7e25d0894868f85125174c2f387ef3236fc9ed9bfb3eff"
    /// //          sealFields:
    /// //            - "0xa00000000000000000000000000000000000000000000000000000000000000000"
    /// //            - "0x880000000000000000"
    /// //          sha3Uncles: "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
    /// //          size: "0x74b"
    /// //          stateRoot: "0xf44699575afd2668060be5ba77e66e1e80edb77ad1b5070969ddfa63da6a4910"
    /// //          timestamp: "0x605aec86"
    /// //          totalDifficulty: "0x6564de"
    /// //          transactions:
    /// //            - "0xcb7edfdb3229c9beeb418ab1ef1a3c9210ecfb22f0157791c3287085d798da58"
    /// //            - "0x0fb803696521ba109c40b3eecb773c93dc6ee891172af0f620c8d44c05198641"
    /// //            - "0x3ef6725cab4470889c3c7d53609a5d4b263701f5891aa98c9ed48b73b6b2fb75"
    /// //            - "0x4010c4c112514756dcdcf14f91117503826dcbe15b03a1636c07aa713da24b8d"
    /// //            - "0xd9c14daa5e2e9cc955534865365ef6bde3045c70e3a984a74c298606c4d67bb5"
    /// //            - "0xfa2326237ba5dcca2127241562be16b68c48fed93d29add8d62f79a00518c2d8"
    /// //          transactionsRoot: "0xddbbd7bf723abdfe885539406540671c2c0eb97684972175ad199258c75416cc"
    /// //          uncles: []
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBlockByHashWithTx(blockHash: String) -> Future<EthBlockdata?> {
        return execAndConvertOptional(in3: in3, method: "eth_getBlockByHash", params:RPCObject( blockHash),  RPCObject(false), convertWith: { try EthBlockdata($0,$1) } )
    }

    /// returns the number of transactions. For Spec, see [eth_getBlockTransactionCountByHash](https://eth.wiki/json-rpc/API#eth_getBlockTransactionCountByHash).
    /// - Parameter blockHash : the blockHash of the block
    /// - Returns: the number of transactions in the block
    public func getBlockTransactionCountByHash(blockHash: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getBlockTransactionCountByHash", params:RPCObject( blockHash), convertWith: toString )
    }

    /// returns the number of transactions. For Spec, see [eth_getBlockTransactionCountByNumber](https://eth.wiki/json-rpc/API#eth_getBlockTransactionCountByNumber).
    /// - Parameter blockNumber : the blockNumber of the block
    /// - Returns: the number of transactions in the block
    public func getBlockTransactionCountByNumber(blockNumber: UInt64) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getBlockTransactionCountByNumber", params:RPCObject( String(format: "0x%1x", blockNumber)), convertWith: toString )
    }

    /// returns the number of uncles. For Spec, see [eth_getUncleCountByBlockHash](https://eth.wiki/json-rpc/API#eth_getUncleCountByBlockHash).
    /// - Parameter blockHash : the blockHash of the block
    /// - Returns: the number of uncles
    public func getUncleCountByBlockHash(blockHash: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getUncleCountByBlockHash", params:RPCObject( blockHash), convertWith: toString )
    }

    /// returns the number of uncles. For Spec, see [eth_getUncleCountByBlockNumber](https://eth.wiki/json-rpc/API#eth_getUncleCountByBlockNumber).
    /// - Parameter blockNumber : the blockNumber of the block
    /// - Returns: the number of uncles
    public func getUncleCountByBlockNumber(blockNumber: UInt64) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getUncleCountByBlockNumber", params:RPCObject( String(format: "0x%1x", blockNumber)), convertWith: toString )
    }

    /// returns the transaction data.
    /// 
    /// See JSON-RPC-Spec for [eth_getTransactionByBlockHashAndIndex](https://eth.wiki/json-rpc/API#eth_getTransactionByBlockHashAndIndex) for more details.
    /// 
    /// - Parameter blockHash : the blockhash containing the transaction.
    /// - Parameter index : the transactionIndex
    /// - Returns: the transactiondata or `null` if it does not exist
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getTransactionByBlockHashAndIndex(blockHash: "0x4fc08daf8d670a23eba7a1aca1f09591c19147305c64d25e1ddd3dd43ff658ee", index: "0xd5") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          blockHash: "0x4fc08daf8d670a23eba7a1aca1f09591c19147305c64d25e1ddd3dd43ff658ee"
    /// //          blockNumber: "0xb8a4a9"
    /// //          from: "0xcaa6cfc2ca92cabbdbce5a46901ee8b831e00a98"
    /// //          gas: "0xac6b"
    /// //          gasPrice: "0x1bf08eb000"
    /// //          hash: "0xd635a97452d604f735116d9de29ac946e9987a20f99607fb03516ef267ea0eea"
    /// //          input: 0x095ea7b300000000000000000000000...a7640000
    /// //          nonce: "0xa"
    /// //          to: "0x95ad61b0a150d79219dcf64e1e6cc01f0b64c4ce"
    /// //          transactionIndex: "0xd5"
    /// //          value: "0x0"
    /// //          type: "0x0"
    /// //          v: "0x25"
    /// //          r: "0xb18e0928c988d898d3217b145d78439072db15ea7de1005a73cf5feaf01a57d4"
    /// //          s: "0x6b530c2613f543f9e26ef9c27a7986c748fbc856aaeacd6000a8ff46d2a2dd78"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTransactionByBlockHashAndIndex(blockHash: String, index: Int) -> Future<EthTransactiondata> {
        return execAndConvert(in3: in3, method: "eth_getTransactionByBlockHashAndIndex", params:RPCObject( blockHash), RPCObject( String(format: "0x%1x", index)), convertWith: { try EthTransactiondata($0,$1) } )
    }

    /// returns the transaction data.
    /// 
    /// See JSON-RPC-Spec for [eth_getTransactionByBlockNumberAndIndex](https://eth.wiki/json-rpc/API#eth_getTransactionByBlockNumberAndIndex) for more details.
    /// 
    /// - Parameter blockNumber : the block number containing the transaction.
    /// - Parameter index : the transactionIndex
    /// - Returns: the transactiondata or `null` if it does not exist
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getTransactionByBlockNumberAndIndex(blockNumber: "0xb8a4a9", index: "0xd5") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          blockHash: "0x4fc08daf8d670a23eba7a1aca1f09591c19147305c64d25e1ddd3dd43ff658ee"
    /// //          blockNumber: "0xb8a4a9"
    /// //          from: "0xcaa6cfc2ca92cabbdbce5a46901ee8b831e00a98"
    /// //          gas: "0xac6b"
    /// //          gasPrice: "0x1bf08eb000"
    /// //          hash: "0xd635a97452d604f735116d9de29ac946e9987a20f99607fb03516ef267ea0eea"
    /// //          input: 0x095ea7b300000000000000000000000...a7640000
    /// //          nonce: "0xa"
    /// //          to: "0x95ad61b0a150d79219dcf64e1e6cc01f0b64c4ce"
    /// //          transactionIndex: "0xd5"
    /// //          value: "0x0"
    /// //          type: "0x0"
    /// //          v: "0x25"
    /// //          r: "0xb18e0928c988d898d3217b145d78439072db15ea7de1005a73cf5feaf01a57d4"
    /// //          s: "0x6b530c2613f543f9e26ef9c27a7986c748fbc856aaeacd6000a8ff46d2a2dd78"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTransactionByBlockNumberAndIndex(blockNumber: UInt64, index: Int) -> Future<EthTransactiondata> {
        return execAndConvert(in3: in3, method: "eth_getTransactionByBlockNumberAndIndex", params:RPCObject( String(format: "0x%1x", blockNumber)), RPCObject( String(format: "0x%1x", index)), convertWith: { try EthTransactiondata($0,$1) } )
    }

    /// returns the transaction data.
    /// 
    /// See JSON-RPC-Spec for [eth_getTransactionByHash](https://eth.wiki/json-rpc/API#eth_getTransactionByHash) for more details.
    /// 
    /// - Parameter txHash : the transactionHash of the transaction.
    /// - Returns: the transactiondata or `null` if it does not exist
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getTransactionByHash(txHash: "0xe9c15c3b26342e3287bb069e433de48ac3fa4ddd32a31b48e426d19d761d7e9b") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 
    /// //          blockHash: "0x4fc08daf8d670a23eba7a1aca1f09591c19147305c64d25e1ddd3dd43ff658ee"
    /// //          blockNumber: "0xb8a4a9"
    /// //          from: "0xcaa6cfc2ca92cabbdbce5a46901ee8b831e00a98"
    /// //          gas: "0xac6b"
    /// //          gasPrice: "0x1bf08eb000"
    /// //          hash: "0xd635a97452d604f735116d9de29ac946e9987a20f99607fb03516ef267ea0eea"
    /// //          input: 0x095ea7b300000000000000000000000...a7640000
    /// //          nonce: "0xa"
    /// //          to: "0x95ad61b0a150d79219dcf64e1e6cc01f0b64c4ce"
    /// //          transactionIndex: "0xd5"
    /// //          value: "0x0"
    /// //          type: "0x0"
    /// //          v: "0x25"
    /// //          r: "0xb18e0928c988d898d3217b145d78439072db15ea7de1005a73cf5feaf01a57d4"
    /// //          s: "0x6b530c2613f543f9e26ef9c27a7986c748fbc856aaeacd6000a8ff46d2a2dd78"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTransactionByHash(txHash: String) -> Future<EthTransactiondata> {
        return execAndConvert(in3: in3, method: "eth_getTransactionByHash", params:RPCObject( txHash), convertWith: { try EthTransactiondata($0,$1) } )
    }

    /// searches for events matching the given criteria. See [eth_getLogs](https://eth.wiki/json-rpc/API#eth_getLogs) for the spec.
    /// - Parameter filter : The filter criteria for the events.
    public func getLogs(filter: EthFilter) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getLogs", params:RPCObject( filter.toRPCDict()), convertWith: toString )
    }

    /// gets the balance of an account for a given block
    /// - Parameter account : address of the account
    /// - Parameter block : the blockNumber or `latest`
    /// - Returns: the balance
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getBalance(account: "0x2e333ec090f1028df0a3c39a918063443be82b2b") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x20599832af6ec00"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getBalance(account: String, block: UInt64?) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getBalance", params:RPCObject( account), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// gets the nonce or number of transaction sent from this account at a given block
    /// - Parameter account : address of the account
    /// - Parameter block : the blockNumber or  `latest`
    /// - Returns: the nonce
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getTransactionCount(account: "0x2e333ec090f1028df0a3c39a918063443be82b2b") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x5"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getTransactionCount(account: String, block: UInt64?) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getTransactionCount", params:RPCObject( account), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// gets the code of a given contract
    /// - Parameter account : address of the account
    /// - Parameter block : the blockNumber or `latest`
    /// - Returns: the code as hex
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getCode(account: "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 0x6080604052348...6c634300050a0040
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getCode(account: String, block: UInt64? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getCode", params:RPCObject( account), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// gets the storage value of a given key
    /// - Parameter account : address of the account
    /// - Parameter key : key to look for
    /// - Parameter block : the blockNumber or`latest`
    /// - Returns: the value of the storage slot.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getStorageAt(account: "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f", key: "0x0") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = "0x19"
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func getStorageAt(account: String, key: String, block: UInt64? = nil) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_getStorageAt", params:RPCObject( account), RPCObject( key), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// signs and sends a Transaction
    /// - Parameter tx : the transactiondata to send
    /// - Returns: the transactionHash
    public func sendTransaction(tx: EthTransaction) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_sendTransaction", params:RPCObject( tx.toRPCDict()), convertWith: toString )
    }

    /// signs and sends a Transaction, but then waits until the transaction receipt can be verified. Depending on the finality of the nodes, this may take a while, since only final blocks will be signed by the nodes.
    /// - Parameter tx : the transactiondata to send
    /// - Returns: the transactionReceipt
    public func sendTransactionAndWait(tx: EthTransaction) -> Future<EthTransactionReceipt> {
        return execAndConvert(in3: in3, method: "eth_sendTransactionAndWait", params:RPCObject( tx.toRPCDict()), convertWith: { try EthTransactionReceipt($0,$1) } )
    }

    /// sends or broadcasts a prviously signed raw transaction. See [eth_sendRawTransaction](https://eth.wiki/json-rpc/API#eth_sendRawTransaction)
    /// - Parameter tx : the raw signed transactiondata to send
    /// - Returns: the transactionhash
    public func sendRawTransaction(tx: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_sendRawTransaction", params:RPCObject( tx), convertWith: toString )
    }

    /// calculates the gas needed to execute a transaction. for spec see [eth_estimateGas](https://eth.wiki/json-rpc/API#eth_estimateGas)
    /// - Parameter tx : the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
    /// - Parameter block : the blockNumber or  `latest`
    /// - Returns: the amount of gass needed.
    public func estimateGas(tx: EthTransaction, block: UInt64?) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_estimateGas", params:RPCObject( tx.toRPCDict()), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// calls a function of a contract (or simply executes the evm opcodes) and returns the result. for spec see [eth_call](https://eth.wiki/json-rpc/API#eth_call)
    /// - Parameter tx : the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
    /// - Parameter block : the blockNumber or  `latest`
    /// - Returns: the abi-encoded result of the function.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).call(tx: EthTx(to: "0x2736D225f85740f42D17987100dc8d58e9e16252", data: "0x5cf0f3570000000000000000000000000000000000000000000000000000000000000001")) .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = 0x0000000000000000000000000...
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func call(tx: EthTx, block: UInt64?) -> Future<String> {
        return execAndConvert(in3: in3, method: "eth_call", params:RPCObject( tx.toRPCDict()), block == nil ? RPCObject("latest") : RPCObject( String(format: "0x%1x", block!) ), convertWith: toString )
    }

    /// The Receipt of a Transaction. For Details, see [eth_getTransactionReceipt](https://eth.wiki/json-rpc/API#eth_gettransactionreceipt).
    /// - Parameter txHash : the transactionHash
    /// - Returns: the TransactionReceipt or `null`  if it does not exist.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// EthAPI(in3).getTransactionReceipt(txHash: "0x5dc2a9ec73abfe0640f27975126bbaf14624967e2b0b7c2b3a0fb6111f0d3c5e") .observe(using: {
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
    public func getTransactionReceipt(txHash: String) -> Future<EthTransactionReceipt?> {
        return execAndConvertOptional(in3: in3, method: "eth_getTransactionReceipt", params:RPCObject( txHash), convertWith: { try EthTransactionReceipt($0,$1) } )
    }


}
/// transaction to sign
public struct EthTransaction {
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

/// the blockdata, or in case the block with that number does not exist, `null` will be returned.
public struct EthBlockdataWithTxHashes {
    /// Array of transaction hashes
    public var transactions: [String]

    /// the block number. `null` when its pending block.
    public var number: UInt64

    /// hash of the block. `null` when its pending block.
    public var hash: String

    /// hash of the parent block.
    public var parentHash: String

    /// hash of the generated proof-of-work. `null` when its pending block.
    public var nonce: UInt256

    /// SHA3 of the uncles Merkle root in the block.
    public var sha3Uncles: String

    /// the bloom filter for the logs of the block. `null` when its pending block.
    public var logsBloom: String

    /// the root of the transaction trie of the block.
    public var transactionsRoot: String

    /// the root of the final state trie of the block.
    public var stateRoot: String

    /// the root of the receipts trie of the block.
    public var receiptsRoot: String

    /// the address of the beneficiary to whom the mining rewards were given.
    public var miner: String

    /// integer of the difficulty for this block.
    public var difficulty: UInt256

    /// integer of the total difficulty of the chain until this block.
    public var totalDifficulty: UInt256

    /// the "extra data" field of this block.
    public var extraData: String

    /// integer the size of this block in bytes.
    public var size: UInt64

    /// the maximum gas allowed in this block.
    public var gasLimit: UInt64

    /// the total used gas by all transactions in this block.
    public var gasUsed: UInt64

    /// the unix timestamp for when the block was collated.
    public var timestamp: UInt64

    /// Array of uncle hashes.
    public var uncles: [String]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        transactions = try toArray(obj["transactions"])!.map({ try toString($0,false)! })
        number = try toUInt64(obj["number"],false)!
        hash = try toString(obj["hash"],false)!
        parentHash = try toString(obj["parentHash"],false)!
        nonce = try toUInt256(obj["nonce"],false)!
        sha3Uncles = try toString(obj["sha3Uncles"],false)!
        logsBloom = try toString(obj["logsBloom"],false)!
        transactionsRoot = try toString(obj["transactionsRoot"],false)!
        stateRoot = try toString(obj["stateRoot"],false)!
        receiptsRoot = try toString(obj["receiptsRoot"],false)!
        miner = try toString(obj["miner"],false)!
        difficulty = try toUInt256(obj["difficulty"],false)!
        totalDifficulty = try toUInt256(obj["totalDifficulty"],false)!
        extraData = try toString(obj["extraData"],false)!
        size = try toUInt64(obj["size"],false)!
        gasLimit = try toUInt64(obj["gasLimit"],false)!
        gasUsed = try toUInt64(obj["gasUsed"],false)!
        timestamp = try toUInt64(obj["timestamp"],false)!
        uncles = try toArray(obj["uncles"])!.map({ try toString($0,false)! })
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["number"] = RPCObject(number)
        obj["hash"] = RPCObject(hash)
        obj["parentHash"] = RPCObject(parentHash)
        obj["nonce"] = RPCObject(nonce)
        obj["sha3Uncles"] = RPCObject(sha3Uncles)
        obj["logsBloom"] = RPCObject(logsBloom)
        obj["transactionsRoot"] = RPCObject(transactionsRoot)
        obj["stateRoot"] = RPCObject(stateRoot)
        obj["receiptsRoot"] = RPCObject(receiptsRoot)
        obj["miner"] = RPCObject(miner)
        obj["difficulty"] = RPCObject(difficulty)
        obj["totalDifficulty"] = RPCObject(totalDifficulty)
        obj["extraData"] = RPCObject(extraData)
        obj["size"] = RPCObject(size)
        obj["gasLimit"] = RPCObject(gasLimit)
        obj["gasUsed"] = RPCObject(gasUsed)
        obj["timestamp"] = RPCObject(timestamp)
        return obj
    }
}

/// the blockdata, or in case the block with that number does not exist, `null` will be returned.
public struct EthBlockdata {
    /// Array of transaction objects
    public var transactions: [EthTransactiondata]

    /// the block number. `null` when its pending block.
    public var number: UInt64

    /// hash of the block. `null` when its pending block.
    public var hash: String

    /// hash of the parent block.
    public var parentHash: String

    /// hash of the generated proof-of-work. `null` when its pending block.
    public var nonce: UInt256

    /// SHA3 of the uncles Merkle root in the block.
    public var sha3Uncles: String

    /// the bloom filter for the logs of the block. `null` when its pending block.
    public var logsBloom: String

    /// the root of the transaction trie of the block.
    public var transactionsRoot: String

    /// the root of the final state trie of the block.
    public var stateRoot: String

    /// the root of the receipts trie of the block.
    public var receiptsRoot: String

    /// the address of the beneficiary to whom the mining rewards were given.
    public var miner: String

    /// integer of the difficulty for this block.
    public var difficulty: UInt256

    /// integer of the total difficulty of the chain until this block.
    public var totalDifficulty: UInt256

    /// the "extra data" field of this block.
    public var extraData: String

    /// integer the size of this block in bytes.
    public var size: UInt64

    /// the maximum gas allowed in this block.
    public var gasLimit: UInt64

    /// the total used gas by all transactions in this block.
    public var gasUsed: UInt64

    /// the unix timestamp for when the block was collated.
    public var timestamp: UInt64

    /// Array of uncle hashes.
    public var uncles: [String]

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        transactions = try toArray(obj["transactions"])!.map({ try EthTransactiondata($0,false)! })
        number = try toUInt64(obj["number"],false)!
        hash = try toString(obj["hash"],false)!
        parentHash = try toString(obj["parentHash"],false)!
        nonce = try toUInt256(obj["nonce"],false)!
        sha3Uncles = try toString(obj["sha3Uncles"],false)!
        logsBloom = try toString(obj["logsBloom"],false)!
        transactionsRoot = try toString(obj["transactionsRoot"],false)!
        stateRoot = try toString(obj["stateRoot"],false)!
        receiptsRoot = try toString(obj["receiptsRoot"],false)!
        miner = try toString(obj["miner"],false)!
        difficulty = try toUInt256(obj["difficulty"],false)!
        totalDifficulty = try toUInt256(obj["totalDifficulty"],false)!
        extraData = try toString(obj["extraData"],false)!
        size = try toUInt64(obj["size"],false)!
        gasLimit = try toUInt64(obj["gasLimit"],false)!
        gasUsed = try toUInt64(obj["gasUsed"],false)!
        timestamp = try toUInt64(obj["timestamp"],false)!
        uncles = try toArray(obj["uncles"])!.map({ try toString($0,false)! })
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["number"] = RPCObject(number)
        obj["hash"] = RPCObject(hash)
        obj["parentHash"] = RPCObject(parentHash)
        obj["nonce"] = RPCObject(nonce)
        obj["sha3Uncles"] = RPCObject(sha3Uncles)
        obj["logsBloom"] = RPCObject(logsBloom)
        obj["transactionsRoot"] = RPCObject(transactionsRoot)
        obj["stateRoot"] = RPCObject(stateRoot)
        obj["receiptsRoot"] = RPCObject(receiptsRoot)
        obj["miner"] = RPCObject(miner)
        obj["difficulty"] = RPCObject(difficulty)
        obj["totalDifficulty"] = RPCObject(totalDifficulty)
        obj["extraData"] = RPCObject(extraData)
        obj["size"] = RPCObject(size)
        obj["gasLimit"] = RPCObject(gasLimit)
        obj["gasUsed"] = RPCObject(gasUsed)
        obj["timestamp"] = RPCObject(timestamp)
        return obj
    }
}

/// Array of transaction objects
public struct EthTransactiondata {
    /// receipient of the transaction.
    public var to: String

    /// sender or signer of the transaction
    public var from: String

    /// value in wei to send
    public var value: UInt256

    /// the gas to be send along
    public var gas: UInt64

    /// the price in wei for one gas-unit. If not specified it will be fetched using `eth_gasPrice`
    public var gasPrice: UInt64

    /// the current nonce of the sender. If not specified it will be fetched using `eth_getTransactionCount`
    public var nonce: UInt64

    /// blockHash of the block holding this transaction or `null` if still pending.
    public var blockHash: String

    /// blockNumber of the block holding this transaction or `null` if still pending.
    public var blockNumber: UInt64

    /// transactionHash
    public var hash: String

    /// data of the transaaction
    public var input: String

    /// index of the transaaction in the block
    public var transactionIndex: UInt64

    /// recovery-byte of the signature
    public var v: String

    /// x-value of the EC-Point of the signature
    public var r: String

    /// y-value of the EC-Point of the signature
    public var s: String

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        to = try toString(obj["to"],false)!
        from = try toString(obj["from"],false)!
        value = try toUInt256(obj["value"],false)!
        gas = try toUInt64(obj["gas"],false)!
        gasPrice = try toUInt64(obj["gasPrice"],false)!
        nonce = try toUInt64(obj["nonce"],false)!
        blockHash = try toString(obj["blockHash"],false)!
        blockNumber = try toUInt64(obj["blockNumber"],false)!
        hash = try toString(obj["hash"],false)!
        input = try toString(obj["input"],false)!
        transactionIndex = try toUInt64(obj["transactionIndex"],false)!
        v = try toString(obj["v"],false)!
        r = try toString(obj["r"],false)!
        s = try toString(obj["s"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["to"] = RPCObject(to)
        obj["from"] = RPCObject(from)
        obj["value"] = RPCObject(value)
        obj["gas"] = RPCObject(gas)
        obj["gasPrice"] = RPCObject(gasPrice)
        obj["nonce"] = RPCObject(nonce)
        obj["blockHash"] = RPCObject(blockHash)
        obj["blockNumber"] = RPCObject(blockNumber)
        obj["hash"] = RPCObject(hash)
        obj["input"] = RPCObject(input)
        obj["transactionIndex"] = RPCObject(transactionIndex)
        obj["v"] = RPCObject(v)
        obj["r"] = RPCObject(r)
        obj["s"] = RPCObject(s)
        return obj
    }
}

/// The filter criteria for the events.
public struct EthFilter {
    /// Integer block number, or "latest" for the last mined block or "pending", "earliest" for not yet mined transactions.
    public var fromBlock: UInt64?

    /// Integer block number, or "latest" for the last mined block or "pending", "earliest" for not yet mined transactions.
    public var toBlock: UInt64?

    /// Contract address or a list of addresses from which logs should originate.
    public var address: String?

    /// Array of 32 Bytes DATA topics. Topics are order-dependent. Each topic can also be an array of DATA with “or” options.
    public var topics: [String]?

    /// With the addition of EIP-234, blockHash will be a new filter option which restricts the logs returned to the single block with the 32-byte hash blockHash. Using blockHash is equivalent to fromBlock = toBlock = the block number with hash blockHash. If blockHash is present in in the filter criteria, then neither fromBlock nor toBlock are allowed.
    public var blockhash: String?

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        fromBlock = try toUInt64(obj["fromBlock"],true)!
        toBlock = try toUInt64(obj["toBlock"],true)!
        address = try toString(obj["address"],true)!
        if let topics = try toArray(obj["topics"],true) {
          self.topics = try topics.map({ try toString($0,true)! })
        } else {
          self.topics = nil
        }
        blockhash = try toString(obj["blockhash"],true)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["fromBlock"] = fromBlock == nil ? RPCObject.none : RPCObject(fromBlock!)
        obj["toBlock"] = toBlock == nil ? RPCObject.none : RPCObject(toBlock!)
        obj["address"] = address == nil ? RPCObject.none : RPCObject(address!)
        obj["blockhash"] = blockhash == nil ? RPCObject.none : RPCObject(blockhash!)
        return obj
    }
}

/// the transactionReceipt
public struct EthTransactionReceipt {
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
    public var logs: EthLogs

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
        logs = try EthLogs(obj["logs"],false)!
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
}

/// array of events created during execution of the tx
public struct EthLogs {
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
}

/// the tx-object, which is the same as specified in [eth_sendTransaction](https://eth.wiki/json-rpc/API#eth_sendTransaction).
public struct EthTx {
    /// address of the contract
    public var to: String

    /// sender of the address
    public var from: String?

    /// value in wei to send
    public var value: UInt256?

    /// the gas to be send along
    public var gas: UInt64?

    /// the price in wei for one gas-unit. If not specified it will be fetched using `eth_gasPrice`
    public var gasPrice: UInt64?

    /// the current nonce of the sender. If not specified it will be fetched using `eth_getTransactionCount`
    public var nonce: UInt64?

    /// the data-section of the transaction, which includes the functionhash and the abi-encoded arguments
    public var data: String?

    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc, optional) else { return nil }
        to = try toString(obj["to"],false)!
        from = try toString(obj["from"],true)!
        value = try toUInt256(obj["value"],true)!
        gas = try toUInt64(obj["gas"],true)!
        gasPrice = try toUInt64(obj["gasPrice"],true)!
        nonce = try toUInt64(obj["nonce"],true)!
        data = try toString(obj["data"],true)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["to"] = RPCObject(to)
        obj["from"] = from == nil ? RPCObject.none : RPCObject(from!)
        obj["value"] = value == nil ? RPCObject.none : RPCObject(value!)
        obj["gas"] = gas == nil ? RPCObject.none : RPCObject(gas!)
        obj["gasPrice"] = gasPrice == nil ? RPCObject.none : RPCObject(gasPrice!)
        obj["nonce"] = nonce == nil ? RPCObject.none : RPCObject(nonce!)
        obj["data"] = data == nil ? RPCObject.none : RPCObject(data!)
        return obj
    }
}