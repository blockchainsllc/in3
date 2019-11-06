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

// implement the transport and storage handlers
if (typeof fetch === 'function') {
    // for browsers
    in3w.in3_cache = {
        get: key => window.localStorage.getItem('in3.' + key),
        set: (key, value) => window.localStorage.setItem('in3.' + key, value)
    }
    in3w.transport = (url, payload) => fetch(url, {
        method: 'POST',
        mode: 'cors', // makes it possible to access them even from the filesystem.
        headers: { 'Content-Type': 'application/json' },
        body: payload
    }).then(res => {
        if (res.status < 200 || res.status >= 400) throw new Error("Error fetrching" + url + ":" + res.statusText)
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
        in3w.transport = (url, payload) => axios.post(url, JSON.parse(payload), { headers: { 'Content-Type': 'application/json' } })
            .then(res => {
                if (res.status != 200) throw new Error("Invalid satus")
                return JSON.stringify(res.data)
            })
    } catch (xx) {
        // if not we use the raw http-implementation of nodejs
        in3w.transport = (url, payload) => new Promise((resolve, reject) => {
            try {
                const postData = payload;//JSON.stringify(payload);
                const m = require(url.startsWith('https') ? 'https' : 'http')
                const req = m.request(url, {
                    method: 'POST',
                    headers: {
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

let response_counter = 1
const responses = {}
function add_response(s) {
    responses['' + response_counter] = s
    return response_counter++
}
function get_response(n) {
    const r = responses['' + n]
    delete responses['' + n]
    return r
}

// signer-delegate
in3w.sign_js = async (clientPtr, type, message, account) => {
    const c = clients['' + clientPtr]
    if (!c) throw new Error('wrong client ptr')
    if (!c.signer) throw new Error('no signer set to handle signing')
    if (!(await c.signer.hasAccount(account))) throw new Error('unknown account ' + account)
    return await c.signer.sign(message, account, type)
}


const clients = {}



// create a flag ndicating when the wasm was succesfully loaded.
let _in3_listeners = []
in3w.onRuntimeInitialized = _ => {
    const o = _in3_listeners
    _in3_listeners = undefined
    o.forEach(_ => _(true))
}



function throwLastError() {
    const er = in3w.ccall('in3_last_error', 'string', [], []);
    if (er) throw new Error(er);
}
const aliases = { kovan: '0x2a', tobalaba: '0x44d', main: '0x1', ipfs: '0x7d0', mainnet: '0x1', goerli: '0x5' }

/**
 * The incubed client.
 */
class IN3 {

    // since loading the wasm is async, we always need to check whether the was was created before using it.
    async _ensure_ptr() {
        if (this.ptr) return
        if (_in3_listeners)
            await new Promise(r => _in3_listeners.push(r))
        this.ptr = in3w.ccall('in3_create', 'number', [], []);
        clients['' + this.ptr] = this
    }

    // here we are creating the instance lazy, when the first function is called.
    constructor(config) {
        this.config = config
        this.needsSetConfig = !!config
        this.ptr = 0;
        this.eth = new EthAPI(this)
    }

    /**
     * configures the client.
     */
    setConfig(conf) {
        if (conf) {
            if (conf.chainId) conf.chainId = aliases[conf.chainId] || conf.chainId
            this.config = { ...this.config, ...conf }
        }
        this.needsSetConfig = !this.ptr
        if (this.ptr) {
            const r = in3w.ccall('in3_config', 'number', ['number', 'string'], [this.ptr, JSON.stringify(this.config)]);
            if (r) throw new Error("Error setting the config : " + r);
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


    /**
     * sends a request and returns the response.
     */
    async sendRequest(rpc) {
        // ensure we have created the instance.
        if (!this.ptr) await this._ensure_ptr();
        if (this.needsSetConfig) this.setConfig()

        // create the context
        const r = in3w.ccall('in3_create_request', 'number', ['number', 'string'], [this.ptr, JSON.stringify(rpc)]);
        if (!r) throwLastError();

        // now send 
        // we add the pending request with pointer as key.
        function checkResponse(error) {
            // check if it was an error...
            const er = error || in3w.ccall('request_get_error', 'string', ['number'], [r])
            // if not we ask for the result.
            const res = er ? '' : in3w.ccall('request_get_result', 'string', ['number'], [r])

            // we always need to cleanup
            in3w.ccall('in3_free_request', 'void', ['number'], [r])

            // resolve or reject the promise.
            if (er) throw new Error(er)
            else {
                const r = JSON.parse(res)
                if (r) delete r.in3
                return r
            }
        }

        // send the request.
        return in3w.ccall('in3_send_request', 'void', ['number'], [r], { async: true }).then(checkResponse, checkResponse)
    }

    async sendRPC(method, params) {
        const res = await this.sendRequest({ method, params })
        if (res.error) throw new Error(res.error.message || res.error)
        return res.result
    }

    free() {
        if (this.ptr) {
            delete clients['' + this.ptr]
            in3w.ccall('in3_dispose', 'void', ['number'], [this.ptr])
            this.ptr = 0
        }
    }


}
// change the transport
IN3.setTransport = function (fn) {
    in3w.transport = fn
}

// change the transport
IN3.setStorage = function (fn) {
    in3w.in3_cache = fn
}

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

// also support ES6-modules
IN3.default = IN3

// defined the export
if (typeof module !== "undefined")
    module.exports = IN3


