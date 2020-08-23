/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
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
 * the configuration of the IN3-Client. This can be changed at any time.
 * All properties are optional and will be verified when sending the next request.
 */
export declare interface IN3Config {
    /**
     * if true the nodelist will be automaticly updated if the lastBlock is newer.
     * 
     * default: true
     */
    autoUpdateList?: boolean

    /**
     * The chain-id based on EIP-155.
     * or the name of the supported chain.
     * 
     * Currently we support 'mainnet', 'goerli', 'kovan', 'ipfs' and 'local'
     * 
     * While most of the chains use preconfigured chain settings, 
     * 'local' actually uses the local running client turning of proof.
     * 
     * example: '0x1' or 'mainnet' or 'goerli'
     * 
     * default: 'mainnet'
     */
    chainId: string // ^0x[0-9a-fA-F]+$

    /**
     * number of signatures requested. The more signatures, the more security you get, but responses may take longer.
     * 
     * default: 0
     */
    signatureCount?: number

    /**
     * the number in percent needed in order reach finality if you run on a POA-Chain.
     * (% of signature of the validators)
     * 
     * default: 0
     */
    finality?: number

    /**
     * if true, the request should include the codes of all accounts. 
     * Otherwise only the the codeHash is returned. 
     * In this case the client may ask by calling eth_getCode() afterwards
     * 
     * default: false
     */
    includeCode?: boolean

    /**
    * if true, the first request (updating the nodelist) will also fetch the current health status
    * and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.
    * 
    * default: false
    */
    bootWeights?: boolean

    /**
     * max number of attempts in case a response is rejected.
     * Incubed will retry to find a different node giving a verified response.
     * 
     * default: 5
     */
    maxAttempts?: number


    /**
     * if true, the in3-section of the response will be kept and returned. 
     * Otherwise it will be removed after validating the data. 
     * This is useful for debugging or if the proof should be used afterwards.
     * 
     * default: false
     */
    keepIn3?: boolean

    /**
     * the key to sign requests. This is required for payments.
     */
    key?: Hash

    /**
     * the limit of nodes to store in the client. If set a random seed will be picked, which is the base for a deterministic verifiable partial nodelist.
     * 
     * default: 0
     */
    nodeLimit?: number

    /**
     * if false, the requests will not be included in the stats of the nodes ( or marked as intern ). 
     * 
     * default: true
     */
    stats?: boolean

    /**
     * specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection.
     * 
     * default: 5000
     */
    timeout?: number
    /**
     * min stake of the server. Only nodes owning at least this amount will be chosen.
     * 
     * default: 0
     */
    minDeposit: number

    /**
     * a bitmask-value combining the minimal properties as filter for the selected nodes. See https://in3.readthedocs.io/en/develop/spec.html#node-structure for details. 
     */
    nodeProps: number | Hex

    /**
     * if true the nodes should send a proof of the response
     * 
     * default: 'standard'
     */
    proof?: 'none' | 'standard' | 'full'

    /**
     * if specified, the blocknumber *latest* will be replaced by blockNumber- specified value
     * 
     * default: 6
     */
    replaceLatestBlock?: number

    /**
     * the number of request send when getting a first answer
     * 
     * default: 1
     */
    requestCount: number

    /**
     * url of a rpc-endpoints to use. If this is set proof will be turned off and it will be treated like local_chain.
     */
    rpc?: string

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
     * the nodelists per chain. the chain_id will be used as key within the object.
     */
    nodes?: {
        [name: string]: {
            /**
             * the address of the registry contract
             * 
             * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945
             */
            contract?: Address

            /**
             * address of the whiteList contract. (optional, cannot be combined with whiteList)
             * 
             * example: 0xe36179e2286ef405e929C90ad3E70E649B22a945
             */
            whiteListContract?: Address

            /**
             * manuall list of whitelisted addresses. (optional, cannot be combined with whiteListContract)
             * 
             * example: ['0xe36179e2286ef405e929C90ad3E70E649B22a945']
             */
            whiteList?: Address[]

            /**
             * if true the nodelist should be updated. This flag will be set to false after the first successfull update.
             * 
             * default: true
             */
            needsUpdate?: boolean

            /**
             * the list of nodes
             */
            nodeList?: IN3NodeConfig[]

            /**
             * if the client sends a array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
             */
            verifiedHashes?: Hex /* bytes32 */[]

            /**
             * identifier of the registry.
             */
            registryId?: Hex /* bytes32 */[]

            /**
             * average block time (seconds) for this chain.
             * 
             * default: 14
             */
            avgBlockTime?: number

        }
    }

    __CONFIG__
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

/**
 * a Incubed plugin.
 * 
 * Depending on the methods this will register for those actions.
 */
interface IN3Plugin<BigIntType, BufferType> {
    /**
     * this is called when the client is cleaned up.
     * @param client the client object
     */
    term?(client: IN3Generic<BigIntType, BufferType>)

    /**
     * returns address
     * @param client 
     */
    getAccount?(client: IN3Generic<BigIntType, BufferType>)

    /**
     * called for each request. 
     * If the plugin wants to handle the request, this function should return the value or a Promise for the value.
     * If the plugin does not want to handle it, it should rreturn undefined.
     * @param client the current client
     * @param request the rpc-request
     */
    handleRPC?(client: IN3Generic<BigIntType, BufferType>, request: RPCRequest): undefined | Promise<any>
}

export default class IN3Generic<BigIntType, BufferType> {
    /**
     * IN3 config
     */
    public config: IN3Config;
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
     * 
     * @param method the method to call. 
     */
    public sendRPC(method: string, params?: any[]): Promise<any>;

    /**
     * disposes the Client. This must be called in order to free allocated memory!
     */
    public free();

    /**
     * returns a Object, which can be used as Web3Provider.
     * 
     * ```
     * const web3 = new Web3(new IN3().createWeb3Provider())
     * ```
     */
    public createWeb3Provider(): any

    /**
     * the signer, if specified this interface will be used to sign transactions, if not, sending transaction will not be possible.
     */
    public signer: Signer<BigIntType, BufferType>;


    /**
     * changes the transport-function.
     * 
     * @param fn the function to fetch the response for the given url
     */
    public static setTransport(fn: (url: string, payload: string, timeout?: number) => Promise<string>): void

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
    public eth: EthAPI<BigIntType, BufferType>

    __API__

    /**
     * collection of util-functions.
     */
    public util: Utils<BufferType>

    /**
     * rregisters a plugin. The plugin may define methods which will be called by the client.
     * @param plugin the plugin-object to register
     */
    public registerPlugin(plugin: IN3Plugin<BigIntType, BufferType>): void

    /**
     * collection of util-functions.
     */
    public static util: Utils<any>

    public static setConvertBigInt(convert: (any) => any)
    public static setConvertBuffer(convert: (any) => any)
    // public static setConvertBuffer<BufferType>(val: any, len?: number) : BufferType

    /** supporting both ES6 and UMD usage */
    public static default: typeof IN3Generic
}

/**
 * default Incubed client with
 * bigint for big numbers
 * Uint8Array for bytes
 */
export class IN3 extends IN3Generic<bigint, Uint8Array> {

    /**
 * creates a new client.
 * @param config a optional config
 */
    public constructor(config?: Partial<IN3Config>);


    public static setConvertBigInt(convert: (any) => any)
    public static setConvertBuffer(convert: (any) => any)

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


export declare interface Signer<BigIntType, BufferType> {
    /** optiional method which allows to change the transaction-data before sending it. This can be used for redirecting it through a multisig. */
    prepareTransaction?: (client: IN3Generic<BigIntType, BufferType>, tx: Transaction) => Promise<Transaction>

    /** returns true if the account is supported (or unlocked) */
    canSign(address: Address): Promise<boolean>

    /** 
     * signing of any data. 
     * if hashFirst is true the data should be hashed first, otherwise the data is the hash.
     */
    sign: (data: Hex, account: Address, hashFirst?: boolean, ethV?: boolean) => Promise<BufferType>
}

export declare class SimpleSigner<BigIntType, BufferType> implements Signer<BigIntType, BufferType> {
    accounts: {
        [ac: string]: BufferType;
    };
    constructor(...pks: (Hash | BufferType)[]);
    addAccount(pk: Hash): string;
    /** optiional method which allows to change the transaction-data before sending it. This can be used for redirecting it through a multisig. */
    prepareTransaction?: (client: IN3Generic<BigIntType, BufferType>, tx: Transaction) => Promise<Transaction>

    /** returns true if the account is supported (or unlocked) */
    canSign(address: Address): Promise<boolean>

    /** 
     * signing of any data. 
     * if hashFirst is true the data should be hashed first, otherwise the data is the hash.
     */
    sign: (data: Hex, account: Address, hashFirst?: boolean, ethV?: boolean) => Promise<BufferType>
}

/**
 * Collection of different util-functions.
 */
export declare interface Utils<BufferType> {
    // toInputToBuffer(data: Hex | BufferType | number | bigint, len?: number): BufferType 

    createSignatureHash(def: ABI): Hex;

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
    keccak(data: BufferType | Data): BufferType

    /**
     * converts any value to a hex string (with prefix 0x).
     * optionally the target length can be specified (in bytes)
     */
    toHex(data: Hex | BufferType | number | bigint, len?: number): Hex
    /**
     * returns the incubed version.
     */
    getVersion(): string

    /** removes all leading 0 in the hexstring */
    toMinHex(key: string | BufferType | number): string;

    /**
     * converts any value to a Uint8Array.
     * optionally the target length can be specified (in bytes)
     */
    toUint8Array(data: Hex | BufferType | number | bigint, len?: number): BufferType

    /**
     * converts any value to a Buffer.
     * optionally the target length can be specified (in bytes)
     */
    toBuffer(data: Hex | BufferType | number | bigint, len?: number): BufferType

    /**
     * converts any value to a hex string (with prefix 0x).
     * optionally the target length can be specified (in bytes)
     */
    toNumber(data: string | BufferType | number | bigint): number

    /**
     * convert to String
     */
    toUtf8(val: any): string;

    /**
     * create a signature (65 bytes) for the given message and kexy
     * @param pk the private key
     * @param msg the message
     * @param hashFirst if true the message will be hashed first (default:true), if not the message is the hash.
     * @param adjustV if true (default) the v value will be adjusted by adding 27
     */
    ecSign(pk: Hex | BufferType, msg: Hex | BufferType, hashFirst?: boolean, adjustV?: boolean): BufferType

    /**
     * takes raw signature (65 bytes) and splits it into a signature object.
     * @param signature the 65 byte-signature
     * @param message  the message
     * @param hashFirst if true (default) this will be taken as raw-data and will be hashed first.
     */
    splitSignature(signature: Hex | BufferType, message: BufferType | Hex, hashFirst?: boolean): Signature

    /**
     * generates the public address from the private key.
     * @param pk the private key.
     */
    private2address(pk: Hex | BufferType): Address

}

