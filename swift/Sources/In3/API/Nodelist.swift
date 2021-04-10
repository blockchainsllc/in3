/// this is generated file don't edit it manually!

import Foundation

/// special Incubed nodelist-handling functions. Most of those are only used internally.
public class Nodelist {
    internal var in3: In3

    /// initialiazes the Nodelist API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
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
    /// Nodelist(in3).nodes(limit: 2, seed: "0xe9c15c3b26342e3287bb069e433de48ac3fa4ddd32a31b48e426d19d761d7e9b", addresses: []) .observe(using: {
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
    public func nodes(limit: Int? = nil, seed: String? = nil, addresses: [String]? = nil) -> Future<NodeListDefinition> {
        return execAndConvert(in3: in3, method: "in3_nodeList", params:limit == nil ? RPCObject.none : RPCObject( String(format: "0x%1x", limit!) ), seed == nil ? RPCObject.none : RPCObject( seed! ), addresses == nil ? RPCObject.none : RPCObject( addresses! ), convertWith: { try NodeListDefinition($0,$1) } )
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
    /// Nodelist(in3).signBlockHash(blocks: NodelistBlocks(blockNumber: 8770580)) .observe(using: {
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
    public func signBlockHash(blocks: NodelistBlocks) -> Future<NodelistSignBlockHash> {
        return execAndConvert(in3: in3, method: "in3_sign", params:RPCObject( blocks.toRPCDict()), convertWith: { try NodelistSignBlockHash($0,$1) } )
    }

    /// Returns whitelisted in3-nodes addresses. The whitelist addressed are accquired from whitelist contract that user can specify in request params.
    /// - Parameter address : address of whitelist contract
    /// - Returns: the whitelisted addresses
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// Nodelist(in3).whitelist(address: "0x08e97ef0a92EB502a1D7574913E2a6636BeC557b") .observe(using: {
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
    public func whitelist(address: String) -> Future<NodelistWhitelist> {
        return execAndConvert(in3: in3, method: "in3_whitelist", params:RPCObject( address), convertWith: { try NodelistWhitelist($0,$1) } )
    }


}
/// the current nodelist
public struct NodeListDefinition {
    /// a array of node definitions.
    public var nodes: [Node]

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
        nodes = try toArray(obj["nodes"])!.map({ try Node($0,false)! })
        contract = try toString(obj["contract"],false)!
        registryId = try toString(obj["registryId"],false)!
        lastBlockNumber = try toUInt64(obj["lastBlockNumber"],false)!
        totalServer = try toUInt64(obj["totalServer"],false)!
    }

    internal func toRPCDict() -> [String:RPCObject] {
        var obj:[String:RPCObject] = [:]
        obj["contract"] = RPCObject( contract )
        obj["registryId"] = RPCObject( registryId )
        obj["lastBlockNumber"] = RPCObject( String(format: "0x%1x", arguments: [lastBlockNumber]) )
        obj["totalServer"] = RPCObject( String(format: "0x%1x", arguments: [totalServer]) )
        return obj
    }

    /// initialize the NodeListDefinition
    ///
    /// - Parameter nodes : a array of node definitions.
    /// - Parameter contract : the address of the Incubed-storage-contract. The client may use this information to verify that we are talking about the same contract or throw an exception otherwise.
    /// - Parameter registryId : the registryId (32 bytes)  of the contract, which is there to verify the correct contract.
    /// - Parameter lastBlockNumber : the blockNumber of the last change of the list (usually the last event).
    /// - Parameter totalServer : the total numbers of nodes.
    public init(nodes: [Node], contract: String, registryId: String, lastBlockNumber: UInt64, totalServer: UInt64) {
        self.nodes = nodes
        self.contract = contract
        self.registryId = registryId
        self.lastBlockNumber = lastBlockNumber
        self.totalServer = totalServer
    }
}

/// a array of node definitions.
public struct Node {
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
        obj["url"] = RPCObject( url )
        obj["address"] = RPCObject( address )
        obj["index"] = RPCObject( String(format: "0x%1x", arguments: [index]) )
        obj["deposit"] = RPCObject( deposit.hexValue )
        obj["props"] = RPCObject( props )
        obj["timeout"] = RPCObject( String(format: "0x%1x", arguments: [timeout]) )
        obj["registerTime"] = RPCObject( String(format: "0x%1x", arguments: [registerTime]) )
        obj["weight"] = RPCObject( String(format: "0x%1x", arguments: [weight]) )
        obj["proofHash"] = RPCObject( proofHash )
        return obj
    }

    /// initialize the Node
    ///
    /// - Parameter url : the url of the node. Currently only http/https is supported, but in the future this may even support onion-routing or any other protocols.
    /// - Parameter address : the address of the signer
    /// - Parameter index : the index within the nodeList of the contract
    /// - Parameter deposit : the stored deposit
    /// - Parameter props : the bitset of capabilities as described in the [Node Structure](spec.html#node-structure)
    /// - Parameter timeout : the time in seconds describing how long the deposit would be locked when trying to unregister a node.
    /// - Parameter registerTime : unix timestamp in seconds when the node has registered.
    /// - Parameter weight : the weight of a node ( not used yet ) describing the amount of request-points it can handle per second.
    /// - Parameter proofHash : a hash value containing the above values. 
    /// This hash is explicitly stored in the contract, which enables the client to have only one merkle proof 
    /// per node instead of verifying each property as its own storage value. 
    /// The proof hash is build `keccak256( abi.encodePacked( deposit, timeout, registerTime, props, signer, url ))` 
    /// 
    public init(url: String, address: String, index: UInt64, deposit: UInt256, props: String, timeout: UInt64, registerTime: UInt64, weight: UInt64, proofHash: String) {
        self.url = url
        self.address = address
        self.index = index
        self.deposit = deposit
        self.props = props
        self.timeout = timeout
        self.registerTime = registerTime
        self.weight = weight
        self.proofHash = proofHash
    }
}

/// array of requested blocks.
public struct NodelistBlocks {
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
        obj["blockNumber"] = RPCObject( String(format: "0x%1x", arguments: [blockNumber]) )
        if let x = hash { obj["hash"] = RPCObject( x ) }
        return obj
    }

    /// initialize the NodelistBlocks
    ///
    /// - Parameter blockNumber : the blockNumber to sign
    /// - Parameter hash : the expected hash. This is optional and can be used to check if the expected hash is correct, but as a client you should not rely on it, but only on the hash in the signature.
    public init(blockNumber: UInt64, hash: String? = nil) {
        self.blockNumber = blockNumber
        self.hash = hash
    }
}

/// the Array with signatures of all the requires blocks.
public struct NodelistSignBlockHash {
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
        obj["blockHash"] = RPCObject( blockHash )
        obj["block"] = RPCObject( String(format: "0x%1x", arguments: [block]) )
        obj["r"] = RPCObject( r )
        obj["s"] = RPCObject( s )
        obj["v"] = RPCObject( v )
        obj["msgHash"] = RPCObject( msgHash )
        return obj
    }

    /// initialize the NodelistSignBlockHash
    ///
    /// - Parameter blockHash : the blockhash which was signed.
    /// - Parameter block : the blocknumber
    /// - Parameter r : r-value of the signature
    /// - Parameter s : s-value of the signature
    /// - Parameter v : v-value of the signature
    /// - Parameter msgHash : the msgHash signed. This Hash is created with `keccak256( abi.encodePacked( _blockhash,  _blockNumber, registryId ))`
    public init(blockHash: String, block: UInt64, r: String, s: String, v: String, msgHash: String) {
        self.blockHash = blockHash
        self.block = block
        self.r = r
        self.s = s
        self.v = v
        self.msgHash = msgHash
    }
}

/// the whitelisted addresses
public struct NodelistWhitelist {
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
        obj["nodes"] = RPCObject( nodes )
        obj["lastWhiteList"] = RPCObject( String(format: "0x%1x", arguments: [lastWhiteList]) )
        obj["contract"] = RPCObject( contract )
        obj["lastBlockNumber"] = RPCObject( String(format: "0x%1x", arguments: [lastBlockNumber]) )
        obj["totalServer"] = RPCObject( String(format: "0x%1x", arguments: [totalServer]) )
        return obj
    }

    /// initialize the NodelistWhitelist
    ///
    /// - Parameter nodes : array of whitelisted nodes addresses.
    /// - Parameter lastWhiteList : the blockNumber of the last change of the in3 white list event.
    /// - Parameter contract : whitelist contract address.
    /// - Parameter lastBlockNumber : the blockNumber of the last change of the list (usually the last event).
    /// - Parameter totalServer : the total numbers of whitelist nodes.
    public init(nodes: String, lastWhiteList: UInt64, contract: String, lastBlockNumber: UInt64, totalServer: UInt64) {
        self.nodes = nodes
        self.lastWhiteList = lastWhiteList
        self.contract = contract
        self.lastBlockNumber = lastBlockNumber
        self.totalServer = totalServer
    }
}