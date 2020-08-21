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

// implement the transport and storage handlers
/* istanbul ignore next */
if (typeof fetch === 'function') {

    // for browsers
    in3w.in3_cache = {
        get: key => window.localStorage.getItem('in3.' + key),
        set: (key, value) => window.localStorage.setItem('in3.' + key, value)
    }
    in3w.transport = (url, payload, timeout) => Promise.race([
        fetch(url, {
            method: 'POST',
            mode: 'cors', // makes it possible to access them even from the filesystem.
            headers: { 'Content-Type': 'application/json', 'User-Agent': 'in3 wasm ' + getVersion() },
            body: payload
        }),
        new Promise((_, reject) => setTimeout(() => reject(new Error('timeout')), timeout || 30000))]
    ).then(res => {
        if (res.status < 200 || res.status >= 400) throw new Error("Error fetching" + url + ":" + res.statusText)
        return res.text()
    })
}
else {
    // for nodejs
    fs = require('' + 'fs')
    try { fs.mkdirSync('.in3') } catch (x) { }
    in3w.in3_cache = {
        get(key) {
            try {
                return fs.readFileSync('.in3/' + key).toString('hex');
            } catch (x) { }
            return null
        },
        set(key, value) {
            fs.writeFileSync('.in3/' + key, Buffer.from(value, 'hex'))
        }
    }

    try {
        // if axios is available, we use it
        const axios = require('' + 'axios')
        in3w.transport = (url, payload, timeout = 30000) => axios.post(url, JSON.parse(payload), { timeout, headers: { 'Content-Type': 'application/json', 'User-Agent': 'in3 wasm ' + getVersion(), in3: 'wasm ' + getVersion() } })
            .then(res => {
                if (res.status != 200) throw new Error("Invalid satus")
                return JSON.stringify(res.data)
            })
    } catch (xx) {
        // if not we use the raw http-implementation of nodejs
        in3w.transport = (url, payload, timeout = 30000) => new Promise((resolve, reject) => {
            try {
                const postData = payload;//JSON.stringify(payload);
                const m = require(url.startsWith('https') ? 'https' : 'http')
                const req = m.request(url, {
                    method: 'POST',
                    headers: {
                        'User-Agent': 'in3 wasm ' + getVersion(),
                        in3: 'wasm ' + getVersion(),
                        'Content-Type': 'application/json',
                        'Content-Length': Buffer.byteLength(postData)
                    },
                    body: payload // body data type must match "Content-Type" header
                }, (res) => {
                    if (res.statusCode < 200 || res.statusCode >= 400)
                        reject(new Error("Invalid Status (" + res.statusCode + ') from server'))
                    else {
                        res.setEncoding('utf8');
                        let result = ''
                        res.on('data', _ => result += _)
                        res.on('end', () => resolve(result))
                    }
                })
                req.setTimeout(timeout, (e) => reject(new Error('timeout')))
                req.on('error', (e) => reject(new Error(e.message)))
                req.write(postData);
                req.end();

            }
            catch (er) {
                console.error('...ERROR : ', er)
                throw er
            }
        })
    }
}
function getVersion() {
    if (in3w._version) return in3w._version
    in3w._version = in3w.ccall('in3_version', 'string', [], [])
    return in3w._version
}

// keep track of all created client instances
const clients = in3w.clients = {}
in3w.promises = {}
in3w.promiseCount = 0;

// create a flag indicating when the wasm was succesfully loaded.
let _in3_listeners = []
in3w.onRuntimeInitialized = _ => {
    in3w.ccall('wasm_init', 'void', [], []);
    const o = _in3_listeners
    _in3_listeners = undefined
    o.forEach(_ => _(true))
}

// check if the last error was set and throws it.
function throwLastError() {
    const er = in3w.ccall('in3_last_error', 'string', [], []);
    if (er) throw new Error(er + (in3w.sign_js.last_sign_error ? (' : ' + in3w.sign_js.last_sign_error) : ''))
}

/**
 * The incubed client.
 */
class IN3 {

    // since loading the wasm is async, we always need to check whether the wasm was created before using it.
    async _ensure_ptr() {
        if (this.ptr) return
        if (_in3_listeners)
            await new Promise(r => _in3_listeners.push(r))
        let chainId = this.config && this.config.chainId
        if (chainId === 'kovan') chainId = '0x2a'
        if (chainId === 'goerli') chainId = '0x5'
        if (chainId === 'mainnet') chainId = '0x1'
        if (chainId === 'btc') chainId = '0x99'
        if (chainId === 'ewc') chainId = '0xf6'
        this.ptr = in3w.ccall('in3_create', 'number', ['number'], [parseInt(chainId) || 0]);
        clients['' + this.ptr] = this
        this.plugins.forEach(_ => this.registerPlugin(_))
    }

    // here we are creating the instance lazy, when the first function is called.
    constructor(config) {
        const def = { requestCount: 2 }
        this.config = config ? { ...def, ...config } : def
        this.needsSetConfig = !!config
        this.ptr = 0;
        this.eth = new EthAPI(this)
        this.ipfs = new IpfsAPI(this)
        this.btc = new BtcAPI(this)
        this.plugins = []
    }

    /**
     * configures the client.
     */
    setConfig(conf) {
        if (conf) {
            const aliases = { kovan: '0x2a', tobalaba: '0x44d', main: '0x1', ipfs: '0x7d0', mainnet: '0x1', goerli: '0x5', ewc: '0xf6', btc: '0x99' }
            if (conf.chainId) conf.chainId = aliases[conf.chainId] || conf.chainId
            this.config = { ...this.config, ...conf }
        }
        this.needsSetConfig = !this.ptr
        if (this.ptr) {
            const r = in3w.ccall('in3_config', 'number', ['number', 'string'], [this.ptr, JSON.stringify(this.config)]);
            if (r) {
                const ex = new Error(UTF8ToString(r))
                _free(r)
                throw ex
            }
        }
    }

    /**
       * sends one or a multiple requests.
       * if the request is a array the response will be a array as well.
       * If the callback is given it will be called with the response, if not a Promise will be returned.
       * This function supports callback so it can be used as a Provider for the web3.
       */
    send(request, callback) {
        const p = this.sendRequest(request)
        if (callback)
            p.then(_ => callback(null, _), err => callback(err, null))
        else
            return p
    }

    registerPlugin(plgn) {
        let action = 0
        if (plgn.term) action |= 0x2
        if (plgn.getAccount) action |= 0x20
        if (plgn.handleRPC) action |= 0x100
        if (plgn.verifyRPC) action |= 0x200
        if (plgn.cacheGet) action |= 0x800
        if (plgn.cacheSet) action |= 0x400
        if (plgn.cacheClear) action |= 0x1000
        let index = this.plugins.indexOf(plgn)
        if (index == -1) {
            index = this.plugins.length
            this.plugins.push(plgn)
        }

        if (this.ptr)
            in3w.ccall('wasm_register_plugin', 'number', ['number', 'number', 'number'], [this.ptr, action, index]);
    }


    /**
     * sends a request and returns the response.
     */
    async sendRequest(rpc) {
        // ensure we have created the instance.
        if (!this.ptr) await this._ensure_ptr();
        if (this.needsSetConfig) this.setConfig()

        // currently we don't handle bulks directly
        if (Array.isArray(rpc)) return Promise.all(rpc.map(_ => this.sendRequest(_)))

        // replace ens-addresses
        if (rpc.params && rpc.method != 'in3_ens')
            for (const p of rpc.params) {
                if (typeof (p) === 'string' && p.endsWith('.eth')) rpc.params[rpc.indexOf(p)] = await this.eth.resolveENS(p)
            }

        // create the context
        let responses = {}
        const r = in3w.ccall('in3_create_request_ctx', 'number', ['number', 'string'], [this.ptr, JSON.stringify(rpc)]);
        if (!r) throwLastError();
        this.pending = (this.pending || 0) + 1

        try {
            // main async loop
            // we repeat it until we have a result
            while (this.ptr && !this.delayFree) {
                const js = call_string('ctx_execute', r).replace(/\n/g, ' > ')
                let state;
                try {
                    state = JSON.parse(js)
                }
                catch (x) {
                    throw new Error("Invalid json:", js)
                }
                switch (state.status) {
                    case 'error':
                        throw new Error(state.error || 'Unknown error')
                    case 'ok':
                        if (Array.isArray(state.result)) {
                            const s = state.result[0]
                            delete s.in3
                            return s
                        }
                        return state.result
                    case 'waiting':
                        await getNextResponse(responses, { ...state.request, in3: this, root: r })
                        break
                    case 'request': {
                        const req = { ...state.request, in3: this, root: r }
                        switch (req.type) {
                            case 'sign':
                                try {
                                    const [message, account] = Array.isArray(req.payload) ? req.payload[0].params : req.payload.params;
                                    if (!this.signer) throw new Error('no signer set to handle signing')
                                    if (!(await this.signer.canSign(account))) throw new Error('unknown account ' + account)
                                    setResponse(req.ctx, toHex(await this.signer.sign(message, account, true, false)), 0, false)
                                } catch (ex) {
                                    setResponse(req.ctx, ex.message || ex, 0, true)
                                }
                                break;

                            case 'rpc':
                                if (req.wait) await new Promise(r => setTimeout(r, req.wait))
                                if (req.urls[0].startsWith("promise://"))
                                    await resolvePromises(req.ctx, req.urls[0])
                                else
                                    await getNextResponse(responses, req)
                        }

                    }

                }
            }
            throw new Error('Request canceled by calling free on the client')
        }
        finally {
            cleanUpResponses(responses, this.ptr)

            // we always need to cleanup
            in3w.ccall('in3_request_free', 'void', ['number'], [r])

            this.pending--
            if (!this.pending && this.delayFree) this.free()
        }
    }


    async sendRPC(method, params = []) {
        const res = await this.sendRequest({ method, params })
        if (res.error) throw new Error(res.error.message || res.error)
        return res.result
    }

    createWeb3Provider() { return this }

    free() {
        if (this.pending)
            this.delayFree = true
        else if (this.ptr) {
            in3w.ccall('in3_dispose', 'void', ['number'], [this.ptr])
            delete clients['' + this.ptr]
            this.ptr = 0
        }
    }
}

async function resolvePromises(ctx, url) {
    const pid = url.substr(10)
    const p = in3w.promises[pid]
    if (!p)
        setResponse(ctx, JSON.stringify({ error: { message: 'could not find the requested proomise' } }), 0, false)
    else {
        delete in3w.promises[pid]
        return p
            .then(r => setResponse(ctx, JSON.stringify({ result: r }), 0, false))
            .catch(e => setResponse(ctx, JSON.stringify({ error: { message: e.message || e } }), 0, false))
    }
}

function cleanUpResponses(responses, ptr) {
    Object.keys(responses).forEach(ctx => responses[ctx].cleanUp(ptr))
}

function getNextResponse(map, req) {
    let res = req.urls ? (map[req.ctx + ''] = url_queue(req)) : map[req.ctx + '']
    return res.getNext()
}

function url_queue(req) {
    let counter = 0
    const promises = [], responses = []
    if (req.in3.config.debug) console.log("send req (" + req.ctx + ") to " + req.urls.join() + ' : ', JSON.stringify(req.payload, null, 2))
    req.urls.forEach((url, i) => in3w.transport(url, JSON.stringify(req.payload), req.timeout || 30000).then(
        response => { responses.push({ i, url, response }); trigger() },
        error => { responses.push({ i, url, error }); trigger() }
    ))
    function trigger() {
        while (promises.length && responses.length) {
            const p = promises.shift(), r = responses.shift()
            if (!req.cleanUp && req.in3.ptr && in3w.ccall('in3_is_alive', 'number', ['number', 'number'], [req.root, req.ctx])) {
                if (r.error) {
                    if (req.in3.config.debug) console.error("res err (" + req.ctx + "," + r.url + ") : " + r.error)
                    setResponse(req.ctx, r.error.message || r.error, r.i, true)
                }
                else {
                    try {
                        if (req.in3.config.debug) console.log("res (" + req.ctx + "," + r.url + ") : " + JSON.stringify(JSON.parse(r.response), null, 2))
                        setResponse(req.ctx, r.response, r.i, false)
                    } catch (x) {
                        setResponse(req.ctx, r.error.message || r.error, r.i, true)
                    }
                }
            }
            p.resolve(r)
        }
    }

    const result = {
        req,
        getNext: () => new Promise((resolve, reject) => {
            counter++
            if (counter > req.urls.length) throw new Error('no more response available')
            promises.push({ resolve, reject })
            trigger()
        }),
        cleanUp(ptr) {
            req.cleanUp = true
            while (req.urls.length - counter > 0) {
                this.getNext().then(
                    r => {
                        // is the client still alive?
                        if (!clients['' + ptr]) return
                        let blacklist = false
                        try {
                            blacklist = r.error || !!JSON.parse(r.response)[0].error
                        }
                        catch (x) {
                            blacklist = true
                        }
                        if (blacklist) in3w.ccall('in3_blacklist', 'void', ['number', 'string'], [ptr, r.url])
                    }, () => { }
                )
            }
        }
    }
    return result
}



// change the transport
IN3.setTransport = function (fn) {
    in3w.transport = fn
}

// change the transport
IN3.setStorage = function (fn) {
    in3w.in3_cache = fn
}

IN3.freeAll = function () {
    Object.keys(clients).forEach(_ => clients[_].free())
}


// the given function fn will be executed as soon as the wasm is loaded. and returns the result as promise.
IN3.onInit = function (fn) {
    return new Promise((resolve, reject) => {
        const check = () => {
            try {
                resolve(fn())
            }
            catch (x) {
                reject(x)
            }
        }
        if (_in3_listeners)
            _in3_listeners.push(check)
        else
            check()
    })
}

// change the Buffer
IN3.setConvertBuffer = function (fn) {
    convertBuffer = fn
}

// also support ES6-modules
IN3.default = IN3

// the default export should be generic (in TS), but the member IN3 uses defaults
IN3.IN3 = IN3

// defined the export
if (typeof module !== "undefined")
    module.exports = IN3



// helper functions
function setResponse(ctx, msg, i, isError) {
    if (msg.length > 5000) {
        // here we pass the string as pointer using malloc before
        const len = (msg.length << 2) + 1;
        const ptr = in3w.ccall('imalloc', 'number', ['number'], [len])
        if (!ptr)
            throw new Error('Could not allocate memory (' + len + ')')
        stringToUTF8(msg, ptr, len);
        in3w.ccall('ctx_set_response', 'void', ['number', 'number', 'number', 'number'], [ctx, i, isError, ptr])
        in3w.ccall('ifree', 'void', ['number'], [ptr])

    }
    else
        in3w.ccall('ctx_set_response', 'void', ['number', 'number', 'number', 'string'], [ctx, i, isError, msg])
    //                        console.log((isError ? 'ERROR ' : '') + ' response  :', msg)
}

function check_ready() {
    if (_in3_listeners) throw new Error('The Incubed wasm runtime is not initialized yet! Please use onInit() to execute it when ready.')
}

