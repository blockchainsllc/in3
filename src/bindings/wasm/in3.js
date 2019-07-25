
if (fetch) {
    in3w.in3_cache = {
        get(key) {
            return window.localStorage.getItem(key)
        },
        set(key, value) {
            return window.localStorage.setItem(key, value)
        }
    }
    in3w.transport = (url, payload) => {
        return fetch(url, {
            method: 'POST', // *GET, POST, PUT, DELETE, etc.
            mode: 'cors', // no-cors, cors, *same-origin
            cache: 'no-cache', // *default, no-cache, reload, force-cache, only-if-cached
            headers: {
                'Content-Type': 'application/json'
            },
            body: payload // body data type must match "Content-Type" header
        }).then(res => {
            if (res.status < 200 || res.status >= 400) throw new Error("Error fetrching" + url + ":" + res.statusText)
            return res.text()
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
    ptr = 0;
    static fn = {};

    async _ensure_ptr() {
        while (!this.ptr) {
            this.ptr = _in3_ready ? in3w.ccall('in3_create', 'number', [], []) : 0;
            if (!this.ptr) await new Promise((res => setTimeout(res, 50)))
        }
    }

    constructor() {
        this.ptr = 0;
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




if (typeof module !== "undefined")
    module.exports = IN3


