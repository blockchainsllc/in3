/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

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
export declare interface RPCRequest {
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
export declare interface RPCResponse {
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


export class IN3 {

    /**
     * creates a new client.
     * @param config a optional config
     */
    public constructor(config?: Partial<IN3Config>);

    /**
     * sets configuration properties. You can pass a partial object specifieing any of defined properties.
     */
    public setConfig(config: Partial<IN3Config>): void;

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

    /**
     * the signer, if specified this interface will be used to sign transactions, if not, sending transaction will not be possible.
     */
    public signer: Signer;


    /**
     * changes the transport-function.
     * 
     * @param fn the function to fetch the response for the given url
     */
    public static setTransport(fn: (url: string, payload: string) => Promise<string>): void

    /**
     * changes the storage handler, which is called to read and write to the cache.
     */
    public static setStorage(handler: {
        get: (key: string) => string,
        set(key: string, value: string): void
    }): void

    /**
     * registers a function to be called as soon as the wasm is ready.
     * If it is already initialized it will call it right away.
     * @param fn the function to call
     * @returns a promise with the result of the function
     */
    public static onInit<T>(fn: () => T): Promise<T>


    /**
     * frees all Incubed instances.
     */
    public static freeAll(): void


    /**
     * eth1 API.
     */
    public eth: EthAPI

    /**
     * collection of util-functions.
     */
    public util: Utils

    /**
     * collection of util-functions.
     */
    public static util: Utils

    /** supporting both ES6 and UMD usage */
    public static default: typeof IN3
}

/**
 * BlockNumber or predefined Block
 */
export type BlockType = number | 'latest' | 'earliest' | 'pending'
/**
 * a Hexcoded String (starting with 0x)
 */
export type Hex = string
/**
 * a BigInteger encoded as hex.
 */
export type Quantity = number | Hex
/**
 * a 32 byte Hash encoded as Hex (starting with 0x)
 */
export type Hash = Hex
/**
 * a 20 byte Address encoded as Hex (starting with 0x)
 */
export type Address = Hex
/**
 * data encoded as Hex (starting with 0x)
 */
export type Data = Hex

/**
 * Signature
 */
export type Signature = {
    message: Data
    messageHash: Hash
    v: Hex
    r: Hash
    s: Hash
    signature?: Data
}

export type ABIField = {
    indexed?: boolean
    name: string
    type: string
}
export type ABI = {
    anonymous?: boolean
    constant?: boolean
    payable?: boolean
    stateMutability?: 'nonpayable' | 'payable' | 'view' | 'pure'

    inputs?: ABIField[],
    outputs?: ABIField[]
    name?: string
    type: 'event' | 'function' | 'constructor' | 'fallback'
}
export type Transaction = {
    /** 20 Bytes - The address the transaction is send from. */
    from: Address
    /** (optional when creating new contract) 20 Bytes - The address the transaction is directed to.*/
    to: Address
    /** Integer of the gas provided for the transaction execution. eth_call consumes zero gas, but this parameter may be needed by some executions. */
    gas: Quantity
    /** Integer of the gas price used for each paid gas.  */
    gasPrice: Quantity
    /** Integer of the value sent with this transaction. */
    value: Quantity
    /** 4 byte hash of the method signature followed by encoded parameters. For details see Ethereum Contract ABI.*/
    data: string
    /** nonce */
    nonce: Quantity
    /** optional chain id */
    chainId?: any
}
export type TransactionReceipt = {
    /** 32 Bytes - hash of the block where this transaction was in. */
    blockHash: Hash
    /** block number where this transaction was in.*/
    blockNumber: BlockType
    /** 20 Bytes - The contract address created, if the transaction was a contract creation, otherwise null.*/
    contractAddress: Address
    /** The total amount of gas used when this transaction was executed in the block. */
    cumulativeGasUsed: Quantity
    /** 20 Bytes - The address of the sender. */
    from: Address
    /** 20 Bytes - The address of the receiver. null when it’s a contract creation transaction.*/
    to: Address
    /** The amount of gas used by this specific transaction alone. */
    gasUsed: Quantity
    /** Array of log objects, which this transaction generated. */
    logs: Log[]
    /** 256 Bytes - A bloom filter of logs/events generated by contracts during transaction execution. Used to efficiently rule out transactions without expected logs.*/
    logsBloom: Data
    /** 32 Bytes - Merkle root of the state trie after the transaction has been executed (optional after Byzantium hard fork EIP609)*/
    root: Hash
    /** 0x0 indicates transaction failure , 0x1 indicates transaction success. Set for blocks mined after Byzantium hard fork EIP609, null before. */
    status: Quantity
    /** 32 Bytes - hash of the transaction. */
    transactionHash: Hash
    /** Integer of the transactions index position in the block. */
    transactionIndex: Quantity
}
export type TransactionDetail = {
    /**  32 Bytes - hash of the transaction. */
    hash: Hash
    /** the number of transactions made by the sender prior to this one.*/
    nonce: Quantity
    /** 32 Bytes - hash of the block where this transaction was in. null when its pending.*/
    blockHash: Hash
    /** block number where this transaction was in. null when its pending.*/
    blockNumber: BlockType
    /** integer of the transactions index position in the block. null when its pending.*/
    transactionIndex: Quantity
    /** 20 Bytes - address of the sender.*/
    from: Address
    /** 20 Bytes - address of the receiver. null when its a contract creation transaction. */
    to: Address
    /**  value transferred in Wei.*/
    value: Quantity
    /** gas price provided by the sender in Wei.*/
    gasPrice: Quantity
    /** gas provided by the sender. */
    gas: Quantity
    /** the data send along with the transaction. */
    input: Data
    /** the standardised V field of the signature.*/
    v: Quantity
    /** the standardised V field of the signature (0 or 1).*/
    standardV: Quantity
    /** the R field of the signature.*/
    r: Quantity
    /** raw transaction data */
    raw: Data
    /** public key of the signer. */
    publicKey: Hash
    /** the chain id of the transaction, if any. */
    chainId: Quantity
    /** creates contract address */
    creates: Address
    /** (optional) conditional submission, Block number in block or timestamp in time or null. (parity-feature)    */
    condition: any
    /** optional: the private key to use for signing */
    pk?: any
}

export type Block = {
    /**  The block number. null when its pending block */
    number: Quantity
    /** hash of the block. null when its pending block */
    hash: Hash
    /** hash of the parent block */
    parentHash: Hash
    /** 8 bytes hash of the generated proof-of-work. null when its pending block. Missing in case of PoA. */
    nonce: Data
    /** SHA3 of the uncles data in the block */
    sha3Uncles: Data
    /** 256 Bytes - the bloom filter for the logs of the block. null when its pending block */
    logsBloom: Data
    /** 32 Bytes - the root of the transaction trie of the block */
    transactionsRoot: Data
    /** 32 Bytes - the root of the final state trie of the block */
    stateRoot: Data
    /** 32 Bytes - the root of the receipts trie of the block */
    receiptsRoot: Data
    /** 20 Bytes - the address of the author of the block (the beneficiary to whom the mining rewards were given)*/
    author: Address
    /** 20 Bytes - alias of ‘author’*/
    miner: Address
    /** integer of the difficulty for this block */
    difficulty: Quantity
    /** integer of the total difficulty of the chain until this block */
    totalDifficulty: Quantity
    /** the ‘extra data’ field of this block */
    extraData: Data
    /** integer the size of this block in bytes */
    size: Quantity
    /** the maximum gas allowed in this block */
    gasLimit: Quantity
    /** the total used gas by all transactions in this block */
    gasUsed: Quantity
    /** the unix timestamp for when the block was collated */
    timestamp: Quantity
    /** Array of transaction objects, or 32 Bytes transaction hashes depending on the last given parameter */
    transactions: (Hash | Transaction)[]
    /** Array of uncle hashes */
    uncles: Hash[]
    /** PoA-Fields */
    sealFields: Data[]
}
export type Log = {
    /** true when the log was removed, due to a chain reorganization. false if its a valid log. */
    removed: boolean
    /** integer of the log index position in the block. null when its pending log. */
    logIndex: Quantity
    /** integer of the transactions index position log was created from. null when its pending log. */
    transactionIndex: Quantity
    /** Hash, 32 Bytes - hash of the transactions this log was created from. null when its pending log. */
    transactionHash: Hash
    /** Hash, 32 Bytes - hash of the block where this log was in. null when its pending. null when its pending log. */
    blockHash: Hash,
    /** the block number where this log was in. null when its pending. null when its pending log. */
    blockNumber: Quantity
    /** 20 Bytes - address from which this log originated. */
    address: Address,
    /**  contains the non-indexed arguments of the log. */
    data: Data
    /** - Array of 0 to 4 32 Bytes DATA of indexed log arguments. (In solidity: The first topic is the hash of the signature of the event (e.g. Deposit(address,bytes32,uint256)), except you declared the event with the anonymous specifier.) */
    topics: Data[]
}

export type LogFilter = {
    /**  Quantity or Tag - (optional) (default: latest) Integer block number, or 'latest' for the last mined block or 'pending', 'earliest' for not yet mined transactions. */
    fromBlock: BlockType
    /** Quantity or Tag - (optional) (default: latest) Integer block number, or 'latest' for the last mined block or 'pending', 'earliest' for not yet mined transactions.*/
    toBlock: BlockType
    /** (optional) 20 Bytes - Contract address or a list of addresses from which logs should originate.*/
    address: Address
    /** (optional) Array of 32 Bytes Data topics. Topics are order-dependent. It’s possible to pass in null to match any topic, or a subarray of multiple topics of which one should be matching. */
    topics: (string | string[])[]
    /** å(optional) The maximum number of entries to retrieve (latest first). */
    limit: Quantity
}

export type TxRequest = {
    /** contract */
    to?: Address

    /** address of the account to use */
    from?: Address

    /** the data to send */
    data?: Data

    /** the gas needed */
    gas?: number

    /** the gasPrice used */
    gasPrice?: number

    /** the nonce */
    nonce?: number

    /** the value in wei */
    value?: Quantity

    /** the ABI of the method to be used */
    method?: string

    /** the argument to pass to the method */
    args?: any[]

    /**raw private key in order to sign */
    pk?: Hash

    /**  number of block to wait before confirming*/
    confirmations?: number
}

export declare interface Signer {
    /** optiional method which allows to change the transaction-data before sending it. This can be used for redirecting it through a multisig. */
    prepareTransaction?: (client: IN3, tx: Transaction) => Promise<Transaction>

    /** returns true if the account is supported (or unlocked) */
    hasAccount(account: Address): Promise<boolean>

    /** 
     * signing of any data. 
     * if hashFirst is true the data should be hashed first, otherwise the data is the hash.
     */
    sign: (data: Hex, account: Address, hashFirst?: boolean, ethV?: boolean) => Promise<Uint8Array>
}

export interface EthAPI {
    client: IN3;
    signer?: Signer;
    constructor(client: IN3);
    /**
     * Returns the number of most recent block. (as number)
     */
    blockNumber(): Promise<number>;
    /**
     * Returns the current price per gas in wei. (as number)
     */
    gasPrice(): Promise<number>;
    /**
     * Executes a new message call immediately without creating a transaction on the block chain.
     */
    call(tx: Transaction, block?: BlockType): Promise<string>;
    /**
     * Executes a function of a contract, by passing a [method-signature](https://github.com/ethereumjs/ethereumjs-abi/blob/master/README.md#simple-encoding-and-decoding) and the arguments, which will then be ABI-encoded and send as eth_call.
     */
    callFn(to: Address, method: string, ...args: any[]): Promise<any>;
    /**
     * Returns the EIP155 chain ID used for transaction signing at the current best block. Null is returned if not available.
     */
    chainId(): Promise<string>;
    /**
     * Makes a call or transaction, which won’t be added to the blockchain and returns the used gas, which can be used for estimating the used gas.
     */
    estimateGas(tx: Transaction): Promise<number>;
    /**
     * Returns the balance of the account of given address in wei (as hex).
     */
    getBalance(address: Address, block?: BlockType): Promise<bigint>;
    /**
     * Returns code at a given address.
     */
    getCode(address: Address, block?: BlockType): Promise<string>;
    /**
     * Returns the value from a storage position at a given address.
     */
    getStorageAt(address: Address, pos: Quantity, block?: BlockType): Promise<string>;
    /**
     * Returns information about a block by hash.
     */
    getBlockByHash(hash: Hash, includeTransactions?: boolean): Promise<Block>;
    /**
     * Returns information about a block by block number.
     */
    getBlockByNumber(block?: BlockType, includeTransactions?: boolean): Promise<Block>;
    /**
     * Returns the number of transactions in a block from a block matching the given block hash.
     */
    getBlockTransactionCountByHash(block: Hash): Promise<number>;
    /**
     * Returns the number of transactions in a block from a block matching the given block number.
     */
    getBlockTransactionCountByNumber(block: Hash): Promise<number>;
    /**
     * Polling method for a filter, which returns an array of logs which occurred since last poll.
     */
    getFilterChanges(id: Quantity): Promise<Log[]>;
    /**
     * Returns an array of all logs matching filter with given id.
     */
    getFilterLogs(id: Quantity): Promise<Log[]>;
    /**
     * Returns an array of all logs matching a given filter object.
     */
    getLogs(filter: LogFilter): Promise<Log[]>;
    /**
     * Returns information about a transaction by block hash and transaction index position.
     */
    getTransactionByBlockHashAndIndex(hash: Hash, pos: Quantity): Promise<TransactionDetail>;
    /**
     * Returns information about a transaction by block number and transaction index position.
     */
    getTransactionByBlockNumberAndIndex(block: BlockType, pos: Quantity): Promise<TransactionDetail>;
    /**
     * Returns the information about a transaction requested by transaction hash.
     */
    getTransactionByHash(hash: Hash): Promise<TransactionDetail>;
    /**
     * Returns the number of transactions sent from an address. (as number)
     */
    getTransactionCount(address: Address, block?: BlockType): Promise<number>;
    /**
     * Returns the receipt of a transaction by transaction hash.
     * Note That the receipt is available even for pending transactions.
     */
    getTransactionReceipt(hash: Hash): Promise<TransactionReceipt>;
    /**
     * Returns information about a uncle of a block by hash and uncle index position.
     * Note: An uncle doesn’t contain individual transactions.
     */
    getUncleByBlockHashAndIndex(hash: Hash, pos: Quantity): Promise<Block>;
    /**
     * Returns information about a uncle of a block number and uncle index position.
     * Note: An uncle doesn’t contain individual transactions.
     */
    getUncleByBlockNumberAndIndex(block: BlockType, pos: Quantity): Promise<Block>;
    /**
     * Returns the number of uncles in a block from a block matching the given block hash.
     */
    getUncleCountByBlockHash(hash: Hash): Promise<number>;
    /**
     * Returns the number of uncles in a block from a block matching the given block hash.
     */
    getUncleCountByBlockNumber(block: BlockType): Promise<number>;
    /**
     * Creates a filter in the node, to notify when a new block arrives. To check if the state has changed, call eth_getFilterChanges.
     */
    newBlockFilter(): Promise<string>;
    /**
     * Creates a filter object, based on filter options, to notify when the state changes (logs). To check if the state has changed, call eth_getFilterChanges.
     *
     * A note on specifying topic filters:
     * Topics are order-dependent. A transaction with a log with topics [A, B] will be matched by the following topic filters:
     *
     * [] “anything”
     * [A] “A in first position (and anything after)”
     * [null, B] “anything in first position AND B in second position (and anything after)”
     * [A, B] “A in first position AND B in second position (and anything after)”
     * [[A, B], [A, B]] “(A OR B) in first position AND (A OR B) in second position (and anything after)”
     */
    newFilter(filter: LogFilter): Promise<string>;
    /**
     * Creates a filter in the node, to notify when new pending transactions arrive.
     *
     * To check if the state has changed, call eth_getFilterChanges.
     */
    newPendingTransactionFilter(): Promise<string>;
    /**
     * Uninstalls a filter with given id. Should always be called when watch is no longer needed. Additonally Filters timeout when they aren’t requested with eth_getFilterChanges for a period of time.
     */
    uninstallFilter(id: Quantity): Promise<Quantity>;
    /**
     * Returns the current ethereum protocol version.
     */
    protocolVersion(): Promise<string>;
    /**
      * Returns the current ethereum protocol version.
      */
    syncing(): Promise<boolean | {
        startingBlock: Hex;
        currentBlock: Hex;
        highestBlock: Hex;
        blockGap: Hex[][];
        warpChunksAmount: Hex;
        warpChunksProcessed: Hex;
    }>;
    /**
     * Creates new message call transaction or a contract creation for signed transactions.
     */
    sendRawTransaction(data: Data): Promise<string>;
    /**
     * signs any kind of message using the `\x19Ethereum Signed Message:\n`-prefix
     * @param account the address to sign the message with (if this is a 32-bytes hex-string it will be used as private key)
     * @param data the data to sign (Buffer, hexstring or utf8-string)
     */
    sign(account: Address, data: Data): Promise<Signature>;
    /** sends a Transaction */
    sendTransaction(args: TxRequest): Promise<string | TransactionReceipt>;
    contractAt(abi: ABI[], address: Address): {
        [methodName: string]: any;
        _address: Address;
        _eventHashes: any;
        events: {
            [event: string]: {
                getLogs: (options: {
                    limit?: number;
                    fromBlock?: BlockType;
                    toBlock?: BlockType;
                    topics?: any[];
                    filter?: {
                        [key: string]: any;
                    };
                }) => Promise<{
                    [key: string]: any;
                    event: string;
                    log: Log;
                }[]>;
            };
            all: {
                getLogs: (options: {
                    limit?: number;
                    fromBlock?: BlockType;
                    toBlock?: BlockType;
                    topics?: any[];
                    filter?: {
                        [key: string]: any;
                    };
                }) => Promise<{
                    [key: string]: any;
                    event: string;
                    log: Log;
                }[]>;
            };
            decode: any;
        };
        _abi: ABI[];
        _in3: IN3;
    };
    decodeEventData(log: Log, d: ABI): any;
    hashMessage(data: Data): Hex;
}
export declare class SimpleSigner implements Signer {
    accounts: {
        [ac: string]: Uint8Array;
    };
    constructor(...pks: (hash | Uint8Array)[]);
    addAccount(pk: Hash): string;
    /** optiional method which allows to change the transaction-data before sending it. This can be used for redirecting it through a multisig. */
    prepareTransaction?: (client: IN3, tx: Transaction) => Promise<Transaction>

    /** returns true if the account is supported (or unlocked) */
    hasAccount(account: Address): Promise<boolean>

    /** 
     * signing of any data. 
     * if hashFirst is true the data should be hashed first, otherwise the data is the hash.
     */
    sign: (data: Hex, account: Address, hashFirst?: boolean, ethV?: boolean) => Promise<Uint8Array>
}

/**
 * Collection of different util-functions.
 */
export declare interface Utils {
    createSignatureHash(def: ABI): Hex;
    createSignature(fields: ABIField[]): string;
    decodeEvent(log: Log, d: ABI): any;
    soliditySha3(...args: any[]): string;

    /**
     * encodes the given arguments as ABI-encoded (including the methodHash)
     * @param signature the method signature
     * @param args the arguments
     */
    abiEncode(signature: string, ...args: any[]): Hex

    /**
     * decodes the given data as ABI-encoded (without the methodHash)
     * @param signature the method signature, which must contain a return description
     * @param data the data to decode
     */
    abiDecode(signature: string, data: Data): any[]

    /**
     * generates a checksum Address for the given address.
     * If the chainId is passed, it will be included accord to EIP 1191
     * @param address the address (as hex)
     * @param chainId the chainId (if supported)
     */
    toChecksumAddress(address: Address, chainId?: number): Address

    /**
     * calculates the keccack hash for the given data.
     * @param data the data as Uint8Array or hex data.
     */
    keccak(data: Uint8Array | Data): Uint8Array

    /**
     * converts any value to a hex string (with prefix 0x).
     * optionally the target length can be specified (in bytes)
     */
    toHex(data: Hex | Uint8Array | number | bigint, len?: number): Hex

    /**
     * converts any value to a Uint8Array.
     * optionally the target length can be specified (in bytes)
     */
    toBuffer(data: Hex | Uint8Array | number | bigint, len?: number): Uint8Array


    /**
     * create a signature (65 bytes) for the given message and kexy
     * @param pk the private key
     * @param msg the message
     * @param hashFirst if true the message will be hashed first (default:true), if not the message is the hash.
     * @param adjustV if true (default) the v value will be adjusted by adding 27
     */
    ecSign(pk: Uint8Array | Hex, msg: Uint8Array | Hex, hashFirst?: boolean, adjustV?: boolean): Uint8Array

    /**
     * takes raw signature (65 bytes) and splits it into a signature object.
     * @param signature the 65 byte-signature
     * @param message  the message
     * @param hashFirst if true (default) this will be taken as raw-data and will be hashed first.
     */
    splitSignature(signature: Uint8Array | Hex, message: Uint8Array | Hex, hashFirst?: boolean): Signature

    /**
     * generates the public address from the private key.
     * @param pk the private key.
     */
    private2address(pk: Hex | Uint8Array): Address

}

export = IN3