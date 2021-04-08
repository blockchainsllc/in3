// This is a generated file, please don't edit it manually!

import Foundation

/// The main Incubed Configuration
public struct In3Config : Codable {

    /// create a new Incubed Client based on the Configuration
    public func createClient() throws -> In3 {
       return try In3(self)
    }


    /// the chainId or the name of a known chain. It defines the nodelist to connect to.
    /// (default: `"mainnet"`)
    /// 
    /// Possible Values are:
    /// 
    /// - `mainnet` : Mainnet Chain
    /// - `goerli` : Goerli Testnet
    /// - `ewc` : Energy WebFoundation
    /// - `btc` : Bitcoin
    /// - `ipfs` : ipfs
    /// - `local` : local-chain
    /// 
    /// 
    /// Example: `goerli`
    public var chainId : String?

    /// the number in percent needed in order reach finality (% of signature of the validators).
    /// 
    /// Example: `50`
    public var finality : Int?

    /// if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards.
    /// 
    /// Example: `true`
    public var includeCode : Bool?

    /// max number of attempts in case a response is rejected.
    /// (default: `7`)
    /// 
    /// Example: `1`
    public var maxAttempts : Int?

    /// if true, requests sent to the input sream of the comandline util will be send theor responses in the same form as the server did.
    /// 
    /// Example: `true`
    public var keepIn3 : Bool?

    /// if true, requests sent will be used for stats.
    /// (default: `true`)
    public var stats : Bool?

    /// if true the client will use binary format. This will reduce the payload of the responses by about 60% but should only be used for embedded systems or when using the API, since this format does not include the propertynames anymore.
    /// 
    /// Example: `true`
    public var useBinary : Bool?

    /// iif true the client allows to use use experimental features, otherwise a exception is thrown if those would be used.
    /// 
    /// Example: `true`
    public var experimental : Bool?

    /// specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection.
    /// (default: `20000`)
    /// 
    /// Example: `100000`
    public var timeout : UInt64?

    /// if true the nodes should send a proof of the response. If set to none, verification is turned off completly.
    /// (default: `"standard"`)
    /// 
    /// Possible Values are:
    /// 
    /// - `none` : no proof will be generated or verfiied. This also works with standard rpc-endpoints.
    /// - `standard` : Stanbdard Proof means all important properties are verfiied
    /// - `full` : In addition to standard, also some rarly needed properties are verfied, like uncles. But this causes a bigger payload.
    /// 
    /// 
    /// Example: `none`
    public var proof : String?

    /// if specified, the blocknumber *latest* will be replaced by blockNumber- specified value.
    /// 
    /// Example: `6`
    public var replaceLatestBlock : Int?

    /// if true the nodelist will be automaticly updated if the lastBlock is newer.
    /// (default: `true`)
    public var autoUpdateList : Bool?

    /// number of signatures requested in order to verify the blockhash.
    /// (default: `1`)
    /// 
    /// Example: `2`
    public var signatureCount : Int?

    /// if true, the first request (updating the nodelist) will also fetch the current health status and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.
    /// (default: `true`)
    /// 
    /// Example: `true`
    public var bootWeights : Bool?

    /// if true the client will try to use http instead of https.
    /// 
    /// Example: `true`
    public var useHttp : Bool?

    /// min stake of the server. Only nodes owning at least this amount will be chosen.
    /// 
    /// Example: `10000000`
    public var minDeposit : UInt256?

    /// used to identify the capabilities of the node.
    /// 
    /// Example: `"0xffff"`
    public var nodeProps : String?

    /// the number of request send in parallel when getting an answer. More request will make it more expensive, but increase the chances to get a faster answer, since the client will continue once the first verifiable response was received.
    /// (default: `2`)
    /// 
    /// Example: `3`
    public var requestCount : Int?

    /// url of one or more direct rpc-endpoints to use. (list can be comma seperated). If this is used, proof will automaticly be turned off.
    /// 
    /// Example: `http://loalhost:8545`
    public var rpc : String?

    /// defining the nodelist. collection of JSON objects with chain Id (hex string) as key.
    /// 
    /// Example: `contract: "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f"
    /// nodeList:
    ///   - address: "0x45d45e6ff99e6c34a235d263965910298985fcfe"
    ///     url: https://in3-v2.slock.it/mainnet/nd-1
    ///     props: "0xFFFF"`
    public var nodes : Nodes?

    /// configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per default).
    /// 
    /// Example: 
    /// ```
    /// account: "0x995628aa92d6a016da55e7de8b1727e1eb97d337"
    /// sync_key: "0x9ad89ac0643ffdc32b2dab859ad0f9f7e4057ec23c2b17699c9b27eff331d816"
    /// signer_type: contract
    /// account: "0x995628aa92d6a016da55e7de8b1727e1eb97d337"
    /// sync_key: "0x9ad89ac0643ffdc32b2dab859ad0f9f7e4057ec23c2b17699c9b27eff331d816"
    /// signer_type: create2
    /// create2:
    ///   creator: "0x6487c3ae644703c1f07527c18fe5569592654bcb"
    ///   saltarg: "0xb90306e2391fefe48aa89a8e91acbca502a94b2d734acc3335bb2ff5c266eb12"
    ///   codehash: "0xd6af3ee91c96e29ddab0d4cb9b5dd3025caf84baad13bef7f2b87038d38251e5"
    /// account: "0x995628aa92d6a016da55e7de8b1727e1eb97d337"
    /// signer_type: pk
    /// musig_pub_keys: 0x9ad89ac0643ffdc32b2dab859ad0f9f7e4057ec23c2b17699c9b27eff331d8160x9ad89ac0643ffdc32b2dab859ad0f9f7e4057ec23c2b17699c9b27eff331d816
    /// sync_key: "0xe8f2ee64be83c0ab9466b0490e4888dbf5a070fd1d82b567e33ebc90457a5734"
    /// musig_urls:
    ///   - null
    ///   - https://approver.service.com
    /// ```
    public var zksync : Zksync?

    /// the client key to sign requests. (only availble if build with `-DPK_SIGNER=true` , which is on per default)
    /// 
    /// Example: `"0xc9564409cbfca3f486a07996e8015124f30ff8331fc6dcbd610a050f1f983afe"`
    public var key : String?

    /// registers raw private keys as signers for transactions. (only availble if build with `-DPK_SIGNER=true` , which is on per default)
    /// 
    /// Example: 
    /// ```
    /// "0xc9564409cbfca3f486a07996e8015124f30ff8331fc6dcbd610a050f1f983afe"
    /// ```
    public var pk : String?

    /// configure the Bitcoin verification
    /// 
    /// Example: `maxDAP: 30
    /// maxDiff: 5`
    public var btc : Btc?

    /// initialize it memberwise
    /// - Parameter chainId : the chainId or the name of a known chain. It defines the nodelist to connect to.
    /// - Parameter finality : the number in percent needed in order reach finality (% of signature of the validators).
    /// - Parameter includeCode : if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards.
    /// - Parameter maxAttempts : max number of attempts in case a response is rejected.
    /// - Parameter keepIn3 : if true, requests sent to the input sream of the comandline util will be send theor responses in the same form as the server did.
    /// - Parameter stats : if true, requests sent will be used for stats.
    /// - Parameter useBinary : if true the client will use binary format. This will reduce the payload of the responses by about 60% but should only be used for embedded systems or when using the API, since this format does not include the propertynames anymore.
    /// - Parameter experimental : iif true the client allows to use use experimental features, otherwise a exception is thrown if those would be used.
    /// - Parameter timeout : specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection.
    /// - Parameter proof : if true the nodes should send a proof of the response. If set to none, verification is turned off completly.
    /// - Parameter replaceLatestBlock : if specified, the blocknumber *latest* will be replaced by blockNumber- specified value.
    /// - Parameter autoUpdateList : if true the nodelist will be automaticly updated if the lastBlock is newer.
    /// - Parameter signatureCount : number of signatures requested in order to verify the blockhash.
    /// - Parameter bootWeights : if true, the first request (updating the nodelist) will also fetch the current health status and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.
    /// - Parameter useHttp : if true the client will try to use http instead of https.
    /// - Parameter minDeposit : min stake of the server. Only nodes owning at least this amount will be chosen.
    /// - Parameter nodeProps : used to identify the capabilities of the node.
    /// - Parameter requestCount : the number of request send in parallel when getting an answer. More request will make it more expensive, but increase the chances to get a faster answer, since the client will continue once the first verifiable response was received.
    /// - Parameter rpc : url of one or more direct rpc-endpoints to use. (list can be comma seperated). If this is used, proof will automaticly be turned off.
    /// - Parameter nodes : defining the nodelist. collection of JSON objects with chain Id (hex string) as key.
    /// - Parameter zksync : configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per default).
    /// - Parameter key : the client key to sign requests. (only availble if build with `-DPK_SIGNER=true` , which is on per default)
    /// - Parameter pk : registers raw private keys as signers for transactions. (only availble if build with `-DPK_SIGNER=true` , which is on per default)
    /// - Parameter btc : configure the Bitcoin verification
    public init(chainId : String? = nil, finality : Int? = nil, includeCode : Bool? = nil, maxAttempts : Int? = nil, keepIn3 : Bool? = nil, stats : Bool? = nil, useBinary : Bool? = nil, experimental : Bool? = nil, timeout : UInt64? = nil, proof : String? = nil, replaceLatestBlock : Int? = nil, autoUpdateList : Bool? = nil, signatureCount : Int? = nil, bootWeights : Bool? = nil, useHttp : Bool? = nil, minDeposit : UInt256? = nil, nodeProps : String? = nil, requestCount : Int? = nil, rpc : String? = nil, nodes : Nodes? = nil, zksync : Zksync? = nil, key : String? = nil, pk : String? = nil, btc : Btc? = nil) {
        self.chainId = chainId
        self.finality = finality
        self.includeCode = includeCode
        self.maxAttempts = maxAttempts
        self.keepIn3 = keepIn3
        self.stats = stats
        self.useBinary = useBinary
        self.experimental = experimental
        self.timeout = timeout
        self.proof = proof
        self.replaceLatestBlock = replaceLatestBlock
        self.autoUpdateList = autoUpdateList
        self.signatureCount = signatureCount
        self.bootWeights = bootWeights
        self.useHttp = useHttp
        self.minDeposit = minDeposit
        self.nodeProps = nodeProps
        self.requestCount = requestCount
        self.rpc = rpc
        self.nodes = nodes
        self.zksync = zksync
        self.key = key
        self.pk = pk
        self.btc = btc
    }

    /// defining the nodelist. collection of JSON objects with chain Id (hex string) as key.
    public struct Nodes : Codable {

        /// address of the registry contract. (This is the data-contract!)
        public var contract : String

        /// address of the whiteList contract. This cannot be combined with whiteList!
        public var whiteListContract : String?

        /// manual whitelist.
        public var whiteList : [String]?

        /// identifier of the registry.
        public var registryId : String

        /// if set, the nodeList will be updated before next request.
        public var needsUpdate : Bool?

        /// average block time (seconds) for this chain.
        public var avgBlockTime : Int?

        /// if the client sends an array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
        public var verifiedHashes : [VerifiedHashes]?

        /// manual nodeList. As Value a array of Node-Definitions is expected.
        public var nodeList : [NodeList]?

        /// initialize it memberwise
        /// - Parameter contract : defining the nodelist. collection of JSON objects with chain Id (hex string) as key.
        /// - Parameter whiteListContract : address of the whiteList contract. This cannot be combined with whiteList!
        /// - Parameter whiteList : manual whitelist.
        /// - Parameter registryId : identifier of the registry.
        /// - Parameter needsUpdate : if set, the nodeList will be updated before next request.
        /// - Parameter avgBlockTime : average block time (seconds) for this chain.
        /// - Parameter verifiedHashes : if the client sends an array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
        /// - Parameter nodeList : manual nodeList. As Value a array of Node-Definitions is expected.
        public init(contract : String, whiteListContract : String? = nil, whiteList : [String]? = nil, registryId : String, needsUpdate : Bool? = nil, avgBlockTime : Int? = nil, verifiedHashes : [VerifiedHashes]? = nil, nodeList : [NodeList]? = nil) {
            self.contract = contract
            self.whiteListContract = whiteListContract
            self.whiteList = whiteList
            self.registryId = registryId
            self.needsUpdate = needsUpdate
            self.avgBlockTime = avgBlockTime
            self.verifiedHashes = verifiedHashes
            self.nodeList = nodeList
        }
    }

    /// if the client sends an array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
    public struct VerifiedHashes : Codable {

        /// block number
        public var block : UInt64

        /// verified hash corresponding to block number.
        public var hash : String

        /// initialize it memberwise
        /// - Parameter block : if the client sends an array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
        /// - Parameter hash : verified hash corresponding to block number.
        public init(block : UInt64, hash : String) {
            self.block = block
            self.hash = hash
        }
    }

    /// manual nodeList. As Value a array of Node-Definitions is expected.
    public struct NodeList : Codable {

        /// URL of the node.
        public var url : String

        /// address of the node
        public var address : String

        /// used to identify the capabilities of the node (defaults to 0xFFFF).
        public var props : String

        /// initialize it memberwise
        /// - Parameter url : manual nodeList. As Value a array of Node-Definitions is expected.
        /// - Parameter address : address of the node
        /// - Parameter props : used to identify the capabilities of the node (defaults to 0xFFFF).
        public init(url : String, address : String, props : String) {
            self.url = url
            self.address = address
            self.props = props
        }
    }

    /// configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per default).
    public struct Zksync : Codable {

        /// url of the zksync-server (if not defined it will be choosen depending on the chain)
        /// (default: `"https://api.zksync.io/jsrpc"`)
        public var provider_url : String?

        /// the account to be used. if not specified, the first signer will be used.
        public var account : String?

        /// the seed used to generate the sync_key. This way you can explicitly set the pk instead of derriving it from a signer.
        public var sync_key : String?

        /// address of the main contract- If not specified it will be taken from the server.
        public var main_contract : String?

        /// type of the account. Must be either `pk`(default), `contract` (using contract signatures) or `create2` using the create2-section.
        /// (default: `"pk"`)
        /// 
        /// Possible Values are:
        /// 
        /// - `pk` : Private matching the account is used ( for EOA)
        /// - `contract` : Contract Signature  based EIP 1271
        /// - `create2` : create2 optionas are used
        /// 
        public var signer_type : String?

        /// concatenated packed public keys (32byte) of the musig signers. if set the pubkey and pubkeyhash will based on the aggregated pubkey. Also the signing will use multiple keys.
        public var musig_pub_keys : String?

        /// a array of strings with urls based on the `musig_pub_keys`. It is used so generate the combined signature by exchaing signature data (commitment and signatureshares) if the local client does not hold this key.
        public var musig_urls : [String]?

        /// create2-arguments for sign_type `create2`. This will allow to sign for contracts which are not deployed yet.
        public var create2 : Create2?

        /// initialize it memberwise
        /// - Parameter provider_url : configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per default).
        /// - Parameter account : the account to be used. if not specified, the first signer will be used.
        /// - Parameter sync_key : the seed used to generate the sync_key. This way you can explicitly set the pk instead of derriving it from a signer.
        /// - Parameter main_contract : address of the main contract- If not specified it will be taken from the server.
        /// - Parameter signer_type : type of the account. Must be either `pk`(default), `contract` (using contract signatures) or `create2` using the create2-section.
        /// - Parameter musig_pub_keys : concatenated packed public keys (32byte) of the musig signers. if set the pubkey and pubkeyhash will based on the aggregated pubkey. Also the signing will use multiple keys.
        /// - Parameter musig_urls : a array of strings with urls based on the `musig_pub_keys`. It is used so generate the combined signature by exchaing signature data (commitment and signatureshares) if the local client does not hold this key.
        /// - Parameter create2 : create2-arguments for sign_type `create2`. This will allow to sign for contracts which are not deployed yet.
        public init(provider_url : String? = nil, account : String? = nil, sync_key : String? = nil, main_contract : String? = nil, signer_type : String? = nil, musig_pub_keys : String? = nil, musig_urls : [String]? = nil, create2 : Create2? = nil) {
            self.provider_url = provider_url
            self.account = account
            self.sync_key = sync_key
            self.main_contract = main_contract
            self.signer_type = signer_type
            self.musig_pub_keys = musig_pub_keys
            self.musig_urls = musig_urls
            self.create2 = create2
        }
    }

    /// create2-arguments for sign_type `create2`. This will allow to sign for contracts which are not deployed yet.
    public struct Create2 : Codable {

        /// The address of contract or EOA deploying the contract ( for example the GnosisSafeFactory )
        public var creator : String

        /// a salt-argument, which will be added to the pubkeyhash and create the create2-salt.
        public var saltarg : String

        /// the hash of the actual deploy-tx including the constructor-arguments.
        public var codehash : String

        /// initialize it memberwise
        /// - Parameter creator : create2-arguments for sign_type `create2`. This will allow to sign for contracts which are not deployed yet.
        /// - Parameter saltarg : a salt-argument, which will be added to the pubkeyhash and create the create2-salt.
        /// - Parameter codehash : the hash of the actual deploy-tx including the constructor-arguments.
        public init(creator : String, saltarg : String, codehash : String) {
            self.creator = creator
            self.saltarg = saltarg
            self.codehash = codehash
        }
    }

    /// configure the Bitcoin verification
    public struct Btc : Codable {

        /// max number of DAPs (Difficulty Adjustment Periods) allowed when accepting new targets.
        /// (default: `20`)
        /// 
        /// Example: `10`
        public var maxDAP : Int?

        /// max increase (in percent) of the difference between targets when accepting new targets.
        /// (default: `10`)
        /// 
        /// Example: `5`
        public var maxDiff : Int?

        /// initialize it memberwise
        /// - Parameter maxDAP : configure the Bitcoin verification
        /// - Parameter maxDiff : max increase (in percent) of the difference between targets when accepting new targets.
        public init(maxDAP : Int? = nil, maxDiff : Int? = nil) {
            self.maxDAP = maxDAP
            self.maxDiff = maxDiff
        }
    }


}
