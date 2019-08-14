
if (typeof fetch === 'function') {
    // browser
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
    // nodejs
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
        const axios = require('' + 'axios')
        in3w.transport = (url, payload) => axios.post(url, JSON.parse(payload), { headers: { 'Content-Type': 'application/json' } })
            .then(res => {
                if (res.status != 200) throw new Error("Invalid satus")
                return JSON.stringify(res.data)
            })
    } catch (xx) {
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
_in3_ready = false;
in3w.onRuntimeInitialized = _ => _in3_ready = true

function throwLastError() {
    const er = in3w.ccall('in3_last_error', 'string', [], []);
    if (er) throw new Error(er);
}

class IN3 {

    async _ensure_ptr() {
        while (!this.ptr) {
            this.ptr = _in3_ready ? in3w.ccall('in3_create', 'number', [], []) : 0;
            if (!this.ptr) await new Promise((res => setTimeout(res, 50)))
        }
    }

    constructor() {
        this.ptr = 0;
    }

    async setConfig(conf) {
        await this._ensure_ptr();
        const r = in3w.ccall('in3_config', 'number', ['number', 'string'], [this.ptr, JSON.stringify(conf)]);
        if (r) throw new Error("Error setting the confiig : " + r);
    }

    async send(rpc) {
        await this._ensure_ptr();

        const r = in3w.ccall('in3_create_request', 'number', ['number', 'string'], [this.ptr, JSON.stringify(rpc)]);
        if (!r) throwLastError();
        in3w.ccall('in3_send_request', 'void', ['number'], [r]);
        return new Promise((resolve, reject) => {
            const check = () => {
                if (in3w.ccall('request_is_done', 'number', ['number'], [r])) {
                    const er = in3w.ccall('request_get_error', 'string', ['number'], [r])
                    const res = er ? '' : in3w.ccall('request_get_result', 'string', ['number'], [r])
                    in3w.ccall('in3_free_request', 'void', ['number'], [r])

                    if (er) reject(new Error(er))
                    else {
                        try {
                            resolve(JSON.parse(res))
                        }
                        catch (ex) {
                            reject(ex)
                        }
                    }
                }
                else
                    setTimeout(check, 50);
            }
            check()
        })
    }

    free() {
        if (this.ptr)
            in3w.ccall('in3_dispose', 'void', ['number'], [this.ptr])
    }
}

IN3.default = IN3

if (typeof module !== "undefined")
    module.exports = IN3


