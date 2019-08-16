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
            fs.writeFile('.in3/' + key, Buffer.from(value, 'hex'), err => err ? console.error('Error caching ' + err) : '')
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



// create a flag ndicating when the wasm was succesfully loaded.
_in3_ready = false;
in3w.onRuntimeInitialized = _ => _in3_ready = true

// for all pending Requests we hold the finalize function which will be called by the wasm when done.
in3w.pendingRequests = {}

function throwLastError() {
    const er = in3w.ccall('in3_last_error', 'string', [], []);
    if (er) throw new Error(er);
}

/**
 * The incubed client.
 */
class IN3 {

    // since loading the wasm is async, we always need to check whether the was was created before using it.
    async _ensure_ptr() {
        while (!this.ptr) {
            this.ptr = _in3_ready ? in3w.ccall('in3_create', 'number', [], []) : 0;
            if (!this.ptr) await new Promise((res => setTimeout(res, 50)))
        }
    }

    // here we are creating the instance lazy, when the first function is called.
    constructor() {
        this.ptr = 0;
    }

    /**
     * configures the client.
     */
    async setConfig(conf) {
        await this._ensure_ptr();
        const r = in3w.ccall('in3_config', 'number', ['number', 'string'], [this.ptr, JSON.stringify(conf)]);
        if (r) throw new Error("Error setting the confiig : " + r);
    }

    /**
     * sends a request and returns the response.
     */
    async send(rpc) {
        // ensure we have created the instance.
        if (!this.ptr) await this._ensure_ptr();

        // create the context
        const r = in3w.ccall('in3_create_request', 'number', ['number', 'string'], [this.ptr, JSON.stringify(rpc)]);
        if (!r) throwLastError();

        // now send 
        return new Promise((resolve, reject) => {
            // we add the pending request with pointer as key.
            in3w.pendingRequests[r + ''] = () => {
                // check if it was an error...
                const er = in3w.ccall('request_get_error', 'string', ['number'], [r])
                // if not we ask for the result.
                const res = er ? '' : in3w.ccall('request_get_result', 'string', ['number'], [r])

                // we always need to cleanup
                in3w.ccall('in3_free_request', 'void', ['number'], [r])
                delete in3w.pendingRequests[r + '']

                // resolve or reject the promise.
                if (er) reject(new Error(er))
                else {
                    try {
                        const r = JSON.parse(res)
                        if (r) delete r.in3
                        resolve(r)
                    }
                    catch (ex) {
                        reject(ex)
                    }
                }
            }

            // send the request.
            in3w.ccall('in3_send_request', 'void', ['number'], [r]);
        })
    }

    free() {
        if (this.ptr)
            in3w.ccall('in3_dispose', 'void', ['number'], [this.ptr])
    }
}

// also support ES6-modules
IN3.default = IN3

// defined the export
if (typeof module !== "undefined")
    module.exports = IN3


