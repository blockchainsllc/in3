/**
 * the iguration of the IN3-Client. This can be paritally overriden for every request.
 */
export declare interface IN3Config {
    /**
     * number of seconds requests can be cached.
     */
    cacheTimeout?: number
    /**
     * the limit of nodes to store in the client.
     * example: 150
     */
    nodeLimit?: number
    /**
     * if true, the in3-section of thr response will be kept. Otherwise it will be removed after validating the data. This is useful for debugging or if the proof should be used afterwards.
     */
    keepIn3?: boolean
    /**
     * the format for sending the data to the client. Default is json, but using cbor means using only 30-40% of the payload since it is using binary encoding
     * example: json
     */
    format?: 'json' | 'jsonRef' | 'cbor'
    /**
     * the client key to sign requests
     * example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
     */
    key?: any
    /**
     * if true the config will be adjusted depending on the request
     */
    autoConfig?: boolean
    /**
     * if true the the request may be handled without proof in case of an error. (use with care!)
     */
    retryWithoutProof?: boolean
    /**
     * max number of attempts in case a response is rejected
     * example: 10
     */
    maxAttempts?: number
    /**
     * if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards
     * example: true
     */
    includeCode?: boolean
    /**
     * number of max bytes used to cache the code in memory
     * example: 100000
     */
    maxCodeCache?: number
    /**
     * number of number of blocks cached  in memory
     * example: 100
     */
    maxBlockCache?: number
    /**
     * if the client sends a array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
     */
    verifiedHashes?: string /* bytes32 */[]
    /**
     * if true the nodes should send a proof of the response
     * example: true
     */
    proof?: 'none' | 'standard' | 'full'
    /**
     * number of signatures requested
     * example: 2
     */
    signatureCount?: number
    /**
     * min stake of the server. Only nodes owning at least this amount will be chosen.
     */
    minDeposit: number
    /**
     * if specified, the blocknumber *latest* will be replaced by blockNumber- specified value
     * example: 6
     */
    replaceLatestBlock?: number
    /**
     * the number of request send when getting a first answer
     * example: 3
     */
    requestCount: number
    /**
     * the number in percent needed in order reach finality (% of signature of the validators)
     * example: 50
     */
    finality?: number
    /**
     * specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection.
     * example: 3000
     */
    timeout?: number
    /**
     * servers to filter for the given chain. The chain-id based on EIP-155.
     * example: 0x1
     */
    chainId: string // ^0x[0-9a-fA-F]+$
    /**
     * main chain-registry contract
     * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945
     */
    chainRegistry?: string // ^0x[0-9a-fA-F]+$
    /**
     * main chain-id, where the chain registry is running.
     * example: 0x1
     */
    mainChain?: string // ^0x[0-9a-fA-F]+$
    /**
     * if true the nodelist will be automaticly updated if the lastBlock is newer
     * example: true
     */
    autoUpdateList?: boolean
    /**
     * a cache handler offering 2 functions ( setItem(string,string), getItem(string) )
     */
    cacheStorage?: any
    /**
     * a url of RES-Endpoint, the client will log all errors to. The client will post to this endpoint JSON like { id?, level, message, meta? }
     */
    loggerUrl?: string
    /**
     * url of one or more rpc-endpoints to use. (list can be comma seperated)
     */
    rpc?: string
    /**
     * the nodelist per chain
     */
    servers?: {
        [name: string]: {
            /**
             * name of the module responsible for handling the verification
             */
            verifier?: string
            /**
             * a alias for the chain
             */
            name?: string
            /**
             * a list of addresses which should always be part of the nodelist when getting an update
             * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945,0x6d17b34aeaf95fee98c0437b4ac839d8a2ece1b1
             */
            initAddresses?: string[]
            /**
             * the blockNumber of the last event in the registry
             * example: 23498798
             */
            lastBlock?: number
            /**
             * the address of the registry contract
             * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945
             */
            contract?: string
            /**
             * if true the nodelist should be updated.
             */
            needsUpdate?: boolean
            /**
             * the chainid for the contract
             * example: 0x8
             */
            contractChain?: string
            /**
             * the list of nodes
             */
            nodeList?: IN3NodeConfig[]
            /**
             * the list of authority nodes for handling conflicts
             * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945,0x6d17b34aeaf95fee98c0437b4ac839d8a2ece1b1
             */
            nodeAuthorities?: string[]
            /**
             * the weights of nodes depending on former performance which is used internally
             */
            weights?: {
                [name: string]: IN3NodeWeight
            }
        }
    }
}
/**
 * a configuration of a in3-server.
 */
export declare interface IN3NodeConfig {
    /**
     * the index within the contract
     * example: 13
     */
    index?: number
    /**
     * the address of the node, which is the public address it iis signing with.
     * example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
     */
    address: string // address
    /**
     * the time (in seconds) until an owner is able to receive his deposit back after he unregisters himself
     * example: 3600
     */
    timeout?: number
    /**
     * the endpoint to post to
     * example: https://in3.slock.it
     */
    url: string
    /**
     * the list of supported chains
     * example: 0x1
     */
    chainIds: string /* hex */[]
    /**
     * the deposit of the node in wei
     * example: 12350000
     */
    deposit: number
    /**
     * the capacity of the node.
     * example: 100
     */
    capacity?: number
    /**
     * the properties of the node.
     * example: 3
     */
    props?: number
    /**
     * the UNIX-timestamp when the node was registered
     * example: 1563279168
     */
    registerTime?: number
    /**
     * the UNIX-timestamp when the node is allowed to be deregister
     * example: 1563279168
     */
    unregisterTime?: number
}
/**
 * a local weight of a n3-node. (This is used internally to weight the requests)
 */
export declare interface IN3NodeWeight {
    /**
     * factor the weight this noe (default 1.0)
     * example: 0.5
     */
    weight?: number
    /**
     * number of uses.
     * example: 147
     */
    responseCount?: number
    /**
     * average time of a response in ms
     * example: 240
     */
    avgResponseTime?: number
    /**
     * last price
     */
    pricePerRequest?: number
    /**
     * timestamp of the last request in ms
     * example: 1529074632623
     */
    lastRequest?: number
    /**
     * blacklisted because of failed requests until the timestamp
     * example: 1529074639623
     */
    blacklistedUntil?: number
}

/**
 * a JSONRPC-Request with N3-Extension
 */
export interface RPCRequest {
    /**
     * the version
     */
    jsonrpc: '2.0'
    /**
     * the method to call
     * example: eth_getBalance
     */
    method: string
    /**
     * the identifier of the request
     * example: 2
     */
    id?: number | string
    /**
     * the params
     * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945,latest
     */
    params?: any[]
}
/**
 * a JSONRPC-Responset with N3-Extension
 */
export interface RPCResponse {
    /**
     * the version
     */
    jsonrpc: '2.0'
    /**
     * the id matching the request
     * example: 2
     */
    id: string | number
    /**
     * in case of an error this needs to be set
     */
    error?: string
    /**
     * the params
     * example: 0xa35bc
     */
    result?: any
}


export default class IN3 {
    /**
     * sets configuration properties. You can pass a partial object specifieing any of defined properties.
     */
    public setConfig(config: Partial<IN3Config>): Promise<void>;

    /**
       * sends a raw request.
       * if the request is a array the response will be a array as well.
       * If the callback is given it will be called with the response, if not a Promise will be returned.
       * This function supports callback so it can be used as a Provider for the web3.
       */
    public send(request: RPCRequest, callback?: (err: Error, response: RPCResponse) => void): Promise<RPCResponse>;

    /**
     * sends a RPC-Requests specified by name and params.
     * 
     * if the response contains an error, this will be thrown. if not the result will be returned.
     */
    public sendRPC(method: string, params: any[]): Promise<any>;

    /**
     * disposes the Client. This must be called in order to free allocated memory!
     */
    public free();
}

