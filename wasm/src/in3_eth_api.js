class EthAPI {

    constructor(client) { this.client = client }

    send(name, ...params) {
        return this.client.sendRPC(name, params || [])
    }
    
    /**
     * Returns the current price per g wei. ()
     */
    async resolveENS(name) {
        if (name && name.endsWith('.eth'))
            return this.send('in3_ens', ...arguments)
        return name
    }

    /**
     * Returns the number of most recent block. ()
     */
    blockNumber() {
        return this.send('eth_blockNumber').then(parseInt)
    }
    /**
     * Returns the current price per g wei. ()
     */
    gasPrice() {
        return this.send('eth_gasPrice').then(parseInt)
    }

    /**
     * Executes a new message call immediately without creating a transaction on the block chain.
     */
    call(tx, block = 'latest') {
        return this.send('eth_call', tx, toHexBlock(block))
    }

    /**
     * Executes a function of a contract, by passing a [method-signature](https://github.com/ethereumjs/ethereumjs-abi/blob/master/README.md#simple-encoding-and-decoding) and the arguments, which will then be ABI-encoded and send _call. 
     */
    async callFn(to, method, ...args) {
        if (typeof (to) === 'string' && to.endsWith('.eth')) to = await this.resolveENS(to)

        if (!method) throw new Error('Missing method-signature')
        const sigPattern = method.match(/(\(.*?\))/g)
        let block = 'latest'
        if (sigPattern && sigPattern.length) {
            const inParams = splitTypes(sigPattern[0])
            if (args.length > inParams.length) {
                block = args[inParams.length]
                args = args.slice(0, inParams.length)
            }
        }
        else throw new Error('Invalid method-signature!')
        return IN3.onInit(() => this.send('eth_call', { to: toHex(to, 20), data: abiEncode(method, ...args) }, block)
            .then(r => {
                const res = abiDecode(method, r)
                return res.length == 1 ? res[0] : res
            }))
    }

    /**
     * Returns the EIP155 chain ID used for transaction signing at the current best block. Null is returned if not available.
     */
    chainId() {
        return this.send('eth_chainId')
    }

    /**
     * Makes a call or transaction, which won’t be added to the blockchain and returns the used gas, which can be used for estimating the used gas.
     */
    estimateGas(tx/*, block = 'latest'*/) {
        return this.send('eth_estimateGas', tx/*, toHexBlock(block)*/).then(parseInt)
    }

    /**
     * Returns the balance of the account of given address in wei ().
     */
    getBalance(address, block = 'latest') {
        return this.send('eth_getBalance', address, toHexBlock(block)).then(toBigInt)
    }

    /**
     * Returns code at a given address.
     */
    getCode(address, block = 'latest') {
        return this.send('eth_getCode', address, block)
    }


    /**
     * Returns the value from a storage position at a given address.
     */
    getStorageAt(address, pos, block = 'latest') {
        return this.send('eth_getStorageAt', address, pos, toHexBlock(block))
    }


    /**
     * Returns information about a block by hash.
     */
    getBlockByHash(hash, includeTransactions = false) {
        return this.send('eth_getBlockByHash', hash, includeTransactions)
    }

    /**
     * Returns information about a block by block number.
     */
    getBlockByNumber(block = 'latest', includeTransactions = false) {
        return this.send('eth_getBlockByNumber', toHexBlock(block), includeTransactions)
    }


    /**
     * Returns the number of transactions in a block from a block matching the given block hash.
     */
    getBlockTransactionCountByHash(block) {
        return this.send('eth_getBlockTransactionCountByHash', block).then(parseInt)
    }


    /**
     * Returns the number of transactions in a block from a block matching the given block number.
     */
    getBlockTransactionCountByNumber(block) {
        return this.send('eth_getBlockTransactionCountByNumber', block).then(parseInt)
    }

    /**
     * Polling method for a filter, which returns an array of logs which occurred since last poll.
     */
    getFilterChanges(id) {
        return this.send('eth_getFilterChanges', id)
    }

    /**
     * Returns an array of all logs matching filter with given id.
     */
    getFilterLogs(id) {
        return this.send('eth_getFilterLogs', id)
    }

    /**
     * Returns an array of all logs matching a given filter object.
     */
    getLogs(filter) {
        if (filter.fromBlock) filter.fromBlock = toHexBlock(filter.fromBlock)
        if (filter.toBlock) filter.toBlock = toHexBlock(filter.toBlock)
        if (filter.limit) filter.limit = toNumber(filter.limit)
        return this.send('eth_getLogs', filter)
    }




    /**
     * Returns information about a transaction by block hash and transaction index position.
     */
    getTransactionByBlockHashAndIndex(hash, pos) {
        return this.send('eth_getTransactionByBlockHashAndIndex', hash, pos)
    }


    /**
     * Returns information about a transaction by block number and transaction index position.
     */
    getTransactionByBlockNumberAndIndex(block, pos) {
        return this.send('eth_getTransactionByBlockNumberAndIndex', toHexBlock(block), pos)
    }

    /**
     * Returns the information about a transaction requested by transaction hash.
     */
    getTransactionByHash(hash) {
        return this.send('eth_getTransactionByHash', hash)
    }

    /**
     * Returns the number of transactions sent from an address. ()
     */
    getTransactionCount(address, block = 'latest') {
        return this.send('eth_getTransactionCount', address, block).then(parseInt)
    }

    /**
     * Returns the receipt of a transaction by transaction hash.
     * Note That the receipt is available even for pending transactions.
     */
    getTransactionReceipt(hash) {
        return this.send('eth_getTransactionReceipt', hash).then(_ => !_ ? null : ({
            ..._,
            contractAddress: _.contractAddress && toChecksumAddress(_.contractAddress),
            from: _.from && toChecksumAddress(_.from)
        }))
    }

    /**
     * Returns information about a uncle of a block by hash and uncle index position.
     * Note: An uncle doesn’t contain individual transactions.
     */
    getUncleByBlockHashAndIndex(hash, pos) {
        return this.send('eth_getUncleByBlockHashAndIndex', hash, pos)
    }


    /**
     * Returns information about a uncle of a block number and uncle index position.
     * Note: An uncle doesn’t contain individual transactions.
     */
    getUncleByBlockNumberAndIndex(block, pos) {
        return this.send('eth_getUncleByBlockNumberAndIndex', block, pos)
    }

    /**
     * Returns the number of uncles in a block from a block matching the given block hash.
     */
    getUncleCountByBlockHash(hash) {
        return this.send('eth_getUncleCountByBlockHash', hash).then(parseInt)
    }

    /**
     * Returns the number of uncles in a block from a block matching the given block hash.
     */
    getUncleCountByBlockNumber(block) {
        return this.send('eth_getUncleCountByBlockNumber', block).then(parseInt)
    }


    /**
     * Creates a filter in the node, to notify when a new block arrives. To check if the state h, call eth_getFilterChanges.
     */
    newBlockFilter() {
        return this.send('eth_newBlockFilter')
    }


    /**
     * Creates a filter object, based on filter options, to notify when the state changes (logs). To check if the state h, call eth_getFilterChanges.
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
    newFilter(filter) {
        return this.send('eth_newFilter', filter)
    }

    /**
     * Creates a filter in the node, to notify when new pending transactions arrive.
     * 
     * To check if the state h, call eth_getFilterChanges.
     */
    newPendingTransactionFilter() {
        return this.send('eth_newPendingTransactionFilter')
    }


    /**
     * Uninstalls a filter with given id. Should always be called when watch is no longer needed. Additonally Filters timeout when they aren’t requested with eth_getFilterChanges for a period of time.
     */
    uninstallFilter(id) {
        return this.send('eth_uninstallFilter', id)
    }

    /**
     * Returns the current ethereum protocol version.
     */
    protocolVersion() {
        return this.send('eth_protocolVersion')
    }

    /**
      * Returns the current ethereum protocol version.
      */
    syncing() {
        return this.send('eth_syncing')
    }


    /**
     * Creates new message call transaction or a contract creation for signed transactions.
     */
    sendRawTransaction(data) {
        return this.send('eth_sendRawTransaction', toHex(data))
    }

    /**
     * signs any kind of message using the `\x19Ethereum Signed Message:\n`-prefix
     * @param account the address to sign the message with (if this is a 32-bytes hex-string it will be used  key)
     * @param data the data to sign (Buffer, hexstring or utf8-string)
     */
    async sign(account, data) {
        // prepare data
        const d = toHex(data),
            message = '0x19' + toHex('Ethereum Signed Message:\n' + (d.length / 2 - 1)).substr(2) + d.substr(2),
            s = {
                message,
                messageHash: keccak(message)
            }

        if (account && account.length == 66) // use direct pk
            s.signature = toHex(ecSign(account, s.messageHash, false))
        else if (this.client.signer && await this.client.signer.canSign(account)) // use signer
            s.signature = toHex(await this.client.signer.sign(s.messageHash, account, false, true))
        else throw new Error('no signer found to sign for this account')
        return { ...splitSignature(s.signature, message, false), ...s, messageHash: toHex(s.messageHash) }
    }

    /** sends a Transaction */
    async sendTransaction(args) {
        if (!args.pk && (!this.client.signer || !(await this.client.signer.canSign(args.from)))) throw new Error('missing signer!')

        // prepare
        const tx = await prepareTransaction(args, this)
        if (args.pk) {
            if (!this.client.signer) this.client.signer = new SimpleSigner(args.pk)
            else if (this.client.signer.__proto__ === SimpleSigner.prototype) this.client.signer.addAccount(args.pk)
            else throw new Error('direct usage of private keys are not possible with a custom signer')
        }
        const txHash = await this.send('eth_sendTransaction', tx)


        if (args.confirmations === undefined) args.confirmations = 1

        // send it
        return args.confirmations ? confirm(txHash, this, parseInt(tx.gas || 21000), args.confirmations) : txHash
    }

    contractAt(abi, address) {
        const api = this, ob = {_address: address, _eventHashes: {}, events: {}, _abi: abi, _in3: this.client }
        for (const def of abi.filter(_ => _.type == 'function')) {
            const method = def.name + createSignature(def.inputs)
            if (def.constant) {
                const signature = method + ':' + createSignature(def.outputs)
                ob[def.name] = function (...args) {
                    return api.callFn(address, signature, ...args)
                        .then(r => {
                            if (def.outputs.length > 1) {
                                let o = {}
                                def.outputs.forEach((d, i) => o[i] = o[d.name] = r[i])
                                return o;
                            }
                            return r
                        })
                }
            }
            else {
                ob[def.name] = function (...args) {
                    let tx = {}
                    if (args.length > def.inputs.length) tx = args.pop()
                    tx.method = method
                    tx.args = args.slice(0, def.name.length);
                    tx.confirmations = tx.confirmations || 1
                    tx.to = address
                    return api.sendTransaction(tx)
                }
            }
            ob[def.name].encode = (...args) => createCallParams(method, args.slice(0, def.name.length)).txdata
        }

        for (const def of abi.filter(_ => _.type == 'event')) {
            const eHash = '0x' + keccak(Buffer.from(def.name + createSignature(def.inputs), 'utf8')).toString('hex')
            ob._eventHashes[def.name] = eHash
            ob._eventHashes[eHash] = def
            ob.events[def.name] = {
                getLogs(options = {}) {
                    return api.getLogs({
                        address,
                        fromBlock: options.fromBlock || 'latest',
                        toBlock: options.toBlock || 'latest',
                        topics: options.topics || [eHash, ...(!options.filter ? [] : def.inputs.filter(_ => _.indexed).map(d => options.filter[d.name] ? '0x' + serialize.bytes32(options.filter[d.name]).toString('hex') : null))],
                        limit: options.limit || 50
                    }).then((logs) => logs.map(_ => {
                        const event = ob.events.decode(_)
                        return { ...event, log: _, event }
                    }))
                }
            }
        }
        ob.events.decode = function (log) { return decodeEventData(log, ob) }
        ob.events.all = {
            getLogs(options = {}) {
                return api.getLogs({
                    address,
                    fromBlock: options.fromBlock || 'latest',
                    toBlock: options.toBlock || 'latest',
                    topics: options.topics || [],
                    limit: options.limit || 50
                }).then((logs) => logs.map(_ => {
                    const event = ob.events.decode(_)
                    return { ...event, log: _, event }
                }))
            }
        }

        return ob
    }

    decodeEventData(log, d) {
        return decodeEvent(log, d)
    }
    hashMessage(data) {
        const d = toHex(data)
        return keccak('0x19' + toHex('Ethereum Signed Message:\n' + (d.length / 2 - 1)) + d.substr(2))
    }


}

async function confirm(txHash, api, gasPaid, confirmations, timeout = 10) {
    let steps = 200
    const start = Date.now()
    while (Date.now() - start < timeout * 1000) {
        const receipt = await api.getTransactionReceipt(txHash)
        if (receipt) {
            if (!receipt.status && gasPaid && gasPaid === parseInt(receipt.gasUsed))
                throw new Error('Transaction failed and all g used up gasPaid=' + gasPaid)
            if (receipt.status && receipt.status == '0x0')
                throw new Error('The Transaction failed because it returned status=0')

            if (confirmations > 1) {
                const start = parseInt(receipt.blockNumber)
                while (start + confirmations - 1 > await api.blockNumber())
                    await new Promise(_ => setTimeout(_, 10))

                return api.getTransactionReceipt(txHash)
            }
            return receipt
        }

        // wait a second and try again
        await new Promise(_ => setTimeout(_, Math.min(timeout * 200, steps *= 2)))
    }

    throw new Error('Error waiting for the transaction to confirm')
}

async function prepareTransaction(args, api) {
    if (typeof (args.to) === 'string' && args.to.endsWith('.eth')) args.to = await api.resolveENS(args.to)
    const sender = args.from || (args.pk && private2address(args.pk))

    const tx = {
        value: util.toMinHex(args.value || 0)
    }
    if (args.to) tx.to = toHex(args.to)
    if (args.method) {
        tx.data = abiEncode(args.method, ...args.args)
        if (args.data) tx.data = args.data + tx.data.substr(10) // this is the case  for deploying contracts
    }
    else if (args.data)
        tx.data = toHex(args.data)
    if (args.nonce) tx.nonce = toMinHex(args.nonce)
    if (args.gasPrice) tx.gasPrice = toMinHex(args.gasPrice)

    if (sender) tx.from = sender
    try {
        tx.gas = util.toMinHex(args.gas || (api && (toNumber(await api.estimateGas(tx)) + 1000) || 3000000))
    }
    catch (ex) {
        throw new Error('The Transaction ' + JSON.stringify(args, null, 2) + ' will not be succesfully executed, since estimating g with: ' + ex)
    }
    return tx
}

function createSignature(fields) {
    return '(' + fields.map(f => {
        let baseType = f.type
        const t = baseType.indexOf('[')
        if (t > 0) baseType = baseType.substr(0, t)
        if (baseType === 'uint' || baseType === 'int') baseType += '256'
        return baseType + (t < 0 ? '' : f.type.substr(t))
    }).join(',') + ')'
}
function parseABIString(def) {
    const [name, args] = def.split(/[\(\)]/)
    return {
        name, type: 'event', inputs: args.split(',').filter(_ => _).map(_ => _.split(' ').filter(z => z)).map(_ => ({
            type: _[0],
            name: _[_.length - 1],
            indexed: _[1] == 'indexed'
        }))
    }
}

function decodeEventData(log, def) {
    let d = (typeof def === 'object') ? def._eventHashes[log.topics[0]] : parseABIString(def)
    if (!d) return null//throw new Error('Could not find the ABI')
    return decodeEvent(log, d)
}

function decodeEvent(log, d) {
    const indexed = d.inputs.filter(_ => _.indexed), unindexed = d.inputs.filter(_ => !_.indexed), r = { event: d && d.name }

    if (indexed.length){
        let logBufs = appendBuffers(log.topics.slice(1).map(_ => toBuffer(_)))
        abiDecode(`prefix():${createSignature(indexed)}`, logBufs).forEach((v, i) => r[indexed[i].name] = v)
    }
    if (unindexed.length)
        abiDecode(`prefix():${createSignature(unindexed)}`, toBuffer(log.data)).forEach((v, i) => r[unindexed[i].name] = v)
    return r
}

/** Concatenating the Typed Buffer arrays together */
function appendBuffers(buffers) {
    const totalLength = buffers.reduce((acc, value) => acc + value.length, 0);
    if (!buffers.length) return null;
    let result = new Uint8Array(totalLength);
  
    // for each array - copy it over result; next array is copied right after the previous one
    let length = 0;
    for(let array of buffers) {
      result.set(array, length);
      length += array.length;
    } 
    return result; 
  }

function toHexBlock(b) {
    return typeof b === 'string' ? b : util.toMinHex(b)
}

function fixBytesValues(input, type) {

    if (type.includes("bytes")) {
        if (!type.includes("[")) {
            return "0x" + util.toBuffer(input, toNumber(type.substr(5))).toString('hex')
        }
        else {
            return (input).map(i => { return ("0x" + util.toBuffer(i, toNumber(type.substr(5))).toString('hex')) })
        }
    }
    else return input
}

function encodeEtheresBN(val) {
    return val && BN.isBN(val) ? toHex(val) : val
}