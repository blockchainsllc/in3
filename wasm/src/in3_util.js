const fixLength = (hex) => hex.length % 2 ? '0' + hex : hex

// Default conversion functions
let convertBigInt = toBigInt
let convertBuffer = toUint8Array



if (typeof (_free) == 'undefined') _free = function (ptr) {
    in3w.ccall("ifree", 'void', ['number'], [ptr])
}
/**
 * internal function calling a wasm-function, which returns a string.
 * @param {*} name 
 * @param  {...any} params_values 
 */
function call_string(name, ...params_values) {
    check_ready()
    const res = in3w.ccall(name, 'number', params_values.map(_ => _ && _.__proto__ === Uint8Array.prototype ? 'array' : typeof _), params_values)
    if (!res) return null
    const result = UTF8ToString(res)
    if (result && result.startsWith(":ERROR:")) throw new Error(result.substr(7))
    _free(res)
    return result
}

function call_buffer(name, len, ...params_values) {
    check_ready()
    const res = in3w.ccall(name, 'number', params_values.map(_ => _ && _.__proto__ === Uint8Array.prototype ? 'array' : typeof _), params_values)
    if (!res) return null
    const result = HEAPU8.slice(res, res + len)
    _free(res)
    return result
}

/**
 *
 * simple promisy-function
 */
function promisify(self, fn, ...args) {
    return new Promise((resolve, reject) => {
        fn.apply(self, [...args, (err, res) => {
            if (err)
                reject(err)
            else
                resolve(res)
        }])
    })
}

// Credit to: https://stackoverflow.com/questions/8936984/uint8array-to-string-in-javascript
function Utf8ArrayToStr(array) {
    var out, i, len, c;
    var char2, char3;

    out = "";
    len = array.length;
    i = 0;
    while (i < len) {
        c = array[i++];
        switch (c >> 4) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                // 0xxxxxxx
                out += String.fromCharCode(c);
                break;
            case 12: case 13:
                // 110x xxxx   10xx xxxx
                char2 = array[i++];
                out += String.fromCharCode(((c & 0x1F) << 6) | (char2 & 0x3F));
                break;
            case 14:
                // 1110 xxxx  10xx xxxx  10xx xxxx
                char2 = array[i++];
                char3 = array[i++];
                out += String.fromCharCode(((c & 0x0F) << 12) |
                    ((char2 & 0x3F) << 6) |
                    ((char3 & 0x3F) << 0));
                break;
        }
    }

    return out;
}

function toUtf8(val) {
    if (!val) return val
    if (val.constructor == Uint8Array) {
        return Utf8ArrayToStr(val)
    }
    if (typeof val === 'string' && val.startsWith('0x')) {
        const hex = fixLength(val).substr(2)
        let str = ''
        for (let i = 0; i < hex.length; i += 2)
            str += String.fromCharCode(parseInt(hex.substr(i, 2), 16))
        return str
    }
    return val.toString('utf8')
}
/**
 * check a RPC-Response for errors and rejects the promise if found
 */
function checkForError(res) {
    if (Array.isArray(res))
        return res.find(_ => !!_.error) ? Promise.reject(new Error(res.find(_ => !!_.error).error)) : res
    return res.error ? Promise.reject(new Error(res.error)) : res
}

/**
 * convert to BigNumber
 */
function toBigInt(val) {
    if (typeof val === 'bigint') return val
    if (typeof val === 'number') return BigInt(val)
    if (typeof val === 'undefined') return undefined
    if (typeof val === 'null') return null
    return BigInt(toHex(val))
}

function keccak(val) {
    if (!val) return val
    val = toUint8Array(val)
    return toBuffer(call_buffer('hash_keccak', 32, val, val.byteLength))
}

function toChecksumAddress(val, chainId = 0) {
    if (!val) return val
    return call_string('to_checksum_address', toUint8Array(val, 20), chainId);
}

function private2address(pk) {
    if (!pk) return pk
    pk = toUint8Array(pk)
    return toChecksumAddress(call_buffer('private_to_address', 20, pk, pk.byteLength))
}

function checkAddressChecksum(ad, chain = 0) {
    return toChecksumAddress(ad, chain = 0) === ad;
}

function abiEncode(sig, ...params) {
    const convert = a => Array.isArray(a)
        ? a.map(convert)
        : (a && a.__proto__ === Object.prototype)
            ? convert(Object.values(a))
            : toHex(a)
    try {
        return call_string('wasm_abi_encode', sig, JSON.stringify(convert(params)))
    }
    catch (x) {
        throw new Error("Error trying to abi encode '" + sig + '": ' + x.message + ' with ' + JSON.stringify(params))
    }
}

function ecSign(pk, data, hashMessage = true, adjustV = true) {
    data = toUint8Array(data)
    pk = toUint8Array(pk)
    return toBuffer(call_buffer('ec_sign', 65, pk, hashMessage ? 1 : 0, data, data.byteLength, adjustV ? 1 : 0))
}

function abiDecode(sig, data) {
    const types_sig = sig.substr(sig.indexOf(':') + 1)
    const types = splitTypes(types_sig)
    const allowOne = types.length == 1 && !types_sig.startsWith('(')
    data = toUint8Array(data)
    if (!types.length) {
        if (data.byteLength) throw new Error('the signature ' + sig + ' does not expect any data but ' + data.byteLength + ' were passed')
        else return []
    }
    try {
        let res = JSON.parse(call_string('wasm_abi_decode', sig, data, data.byteLength))
        return allowOne ? convertType(res, types[0]) : convertTypes(types, res)
    } catch (x) {
        throw new Error('Error decoding ' + sig + ' with ' + toHex(data) + ' : ' + x.message)
    }
}

function convertType(val, t) {
    const isArray = t.indexOf('[')
    if (isArray >= 0) {
        t = t.substr(0, isArray)
        if (t !== 'string' && t != 'bytes')
            return val ? val.map(_ => convertType(_, t)) : []
    }

    if (t.startsWith('(')) return convertTypes(splitTypes(t), val)
    switch (t) {
        case 'bool':
            return !!toNumber(val)
        case 'address':
            return toChecksumAddress(toHex(val, 20))
        case 'string':
            return toUtf8(val)
        case 'bytes':
            return toHex(val)
        case 'uint8':
        case 'uint16':
        case 'uint32':
            return toNumber(val)
        default:
            return t.startsWith('bytes') ? toHex(val) : convertBigInt(val)
    }
}

function convertTypes(types, res) {
    if (types.length != res.length) throw new Error('Mismatch in result def')
    return res.map((val, i) => convertType(val, types[i]))
}

function splitTypes(types, removeBrackets = true) {
    if (removeBrackets && types.startsWith('(') && types.endsWith(')')) types = types.substr(1, types.length - 2)
    let p = 0,
        l = 0,
        res = []
    for (let i = 0; i < types.length; i++)
        switch (types[i]) {
            case '(':
                l++;
                break;
            case ')':
                l--;
                break;
            case ',':
                if (!l) {
                    res.push(types.substring(p, i))
                    p = i + 1
                }
        }
    if (types.length > p + 1) res.push(types.substr(p))
    return res
}

function randomBytes(len) {
    const vals = new Uint8Array(len)
    try {
        return toBuffer(crypto.getRandomValues(vals))
    }
    catch (x) {
        try {
            return toBuffer(require('crypto').randomBytes(len))
        }
        catch (y) {
            const seed = keccak(Date.now())
            for (let i = 0; i < len; i++)
                vals[i] = seed[i % 32] ^ Math.floor(Math.random() * 256)
            return toBuffer(vals)
        }
    }
}

function convertInt2Hex(val) {
    if (val.length < 16)
        return parseInt(val).toString(16)
    else
        return val.startsWith('-')
            ? '-' + call_string('wasm_to_hex', val.substr(1)).substr(2)
            : call_string('wasm_to_hex', val).substr(2)
}

/**
 * converts any value as hex-string
 */
function toHex(val, bytes) {
    if (val === undefined)
        return undefined;
    let hex = ''
    if (typeof val === 'string')
        hex = (val.startsWith('0x') || val.startsWith('0X'))
            ? val.substr(2)
            : (!isNaN(parseInt(val[val[0] == '-' ? 1 : 0])) ? convertInt2Hex(val) : convertUTF82Hex(val))
    else if (typeof val === 'boolean')
        hex = val ? '01' : '00'
    else if (typeof val === 'number' || typeof val === 'bigint')
        hex = val.toString(16)
    else if (val && val._isBigNumber) // BigNumber
        hex = val.toHexString()
    else if (val && (val.redIMul || val.readBigInt64BE)) // bn.js or nodejs Buffer
        hex = val.toString('hex')
    else if (val && val.byteLength) {
        const ar = val.buffer ? val : new Uint8Array(val)
        for (let i = 0; i < val.byteLength; i++) hex += padStart(ar[i].toString(16), 2, '0')
    } else
        throw new Error('Unknown or unsupported type : ' + JSON.stringify(val))

    if (hex.startsWith('-')) {
        let n = new Array(hex.length - 1), o = 0;
        for (let i = hex.length - 1; i > 0; i--) n[hex.length - i] = parseInt(hex[i], 16)
        hex = padStart(n.map(_ => ((!o && !_) ? 0 : (16 - _ - (o || (o++)))).toString(16)).reverse().join(''), (bytes || 32) * 2, 'f')
    }

    if (bytes)
        hex = padStart(hex, bytes * 2, '0'); // workaround for ts-error in older js
    if (hex.length % 2)
        hex = '0' + hex;
    return '0x' + hex.toLowerCase();
}
/**
 * converts to a js-number
 */
function toNumber(val) {
    switch (typeof val) {
        case 'number':
            return val
        case 'boolean':
            return val ? 1 : 0
        case 'bigint':
        case 'string':
            return parseInt(val)
        case 'undefined':
        case 'null':
            return 0
        default:
            if (!val) return 0
            if (val.readBigInt64BE) //nodejs Buffer
                return val.length == 0 ? 0 : parseInt(toMinHex(val))
            else if (val.redIMul)
                return val.bitLength() > 53 ? toNumber(val.toArrayLike(Uint8Array)) : val.toNumber()
            else if (val.byteLength)
                return parseInt(toHex(val))
            else if (val && val._isBigNumber)
                try {
                    return val.toNumber()
                }
                catch (ex) {
                    return toNumber(val.toHexString())
                }
            throw new Error('can not convert a ' + (typeof val) + ' to number');
    }
}

function toBuffer(val, len = -1) {
    return convertBuffer(val, len)
}
/**
 * converts any value as Buffer (toUint8Array)
 *  if len === 0 it will return an empty Buffer if the value is 0 or '0x00', since this is the way rlpencode works wit 0-values.
 */
function toUint8Array(val, len = -1) {
    if (val && val._isBigNumber)
        val = val.toHexString();
    if (typeof val == 'string') {
        let b;
        if (val && !val.startsWith('0x') && val.length && (parseInt(val) || val == '0'))
            val = toHex(val)

        if (val.startsWith('0x')) {
            val = fixLength(val.substr(2))
            b = new Uint8Array(val.length >> 1)
            for (let i = 0; i < b.byteLength; i++) b[i] = parseInt('0x' + val.substr(i << 1, 2))
        } else {
            b = new Uint8Array(val.length)
            for (let i = 0; i < b.byteLength; i++) b[i] = val.charCodeAt(i)
        }
        val = b
    } else if (typeof val == 'number')
        val = val === 0 && len === 0 ? new Uint8Array(0) : toBuffer('0x' + fixLength(val.toString(16)), len);
    else if (val && val.redIMul)
        val = val.toArrayLike(Uint8Array)
    if (!val)
        val = new Uint8Array(0)
    // since rlp encodes an empty array for a 0 -value we create one if the required len===0
    if (len == 0 && val.byteLength == 1 && val[0] === 0)
        return new Uint8Array(0)
    // if we have a defined length, we should padLeft 00 or cut the left content to ensure length
    if (len > 0 && val.byteLength && val.byteLength !== len) {
        if (val.byteLength > len) return val.slice(val.byteLength - len)
        let b = new Uint8Array(len)
        for (let i = 0; i < val.byteLength; i++)
            b[len - val.byteLength + i] = val[i]
        return b
    }
    if (val.__proto__ !== Uint8Array.prototype) {
        let b = new Uint8Array(val.byteLength)
        for (let i = 0; i < val.byteLength; i++) b[i] = val[i]
        return b
    }
    return val;
}
/**
 * removes all leading 0 in a hex-string
 */
function toSimpleHex(val) {
    let hex = val.replace('0x', '');
    while (hex.startsWith('00') && hex.length > 2)
        hex = hex.substr(2);
    return '0x' + hex;
}
/**
 * decodes to base64
 */
function base64Decode(val) {
    // calculate the length
    if ((typeof val) !== 'string') throw new Error('Must be a string as input')
    let lip = val.length
    let len = lip / 4 * 3
    if (lip > 1 && val[lip - 2] == '=' && val[lip - 1] == '=')
        len -= 2
    else if (val[lip - 1] == '=')
        len -= 1

    return call_buffer('base64Decode', len, val)
}
/**
 * encodes to base64
 */
function base64Encode(val) {
    return call_string('base64Encode', val, val.length)
}
/**
 * returns a address from a private key
 */
function getAddress(pk) {
    return private2address(pk)
}
/** removes all leading 0 in the hexstring */
function toMinHex(key) {
    if (typeof key !== 'string' || !key.startsWith('0x'))
        key = toHex(key)
    if (typeof key === 'number')
        key = toHex(key);
    if (typeof key === 'string') {
        key = key.trim();
        if (key.length < 3 || key[0] != '0' || key[1] != 'x')
            throw new Error("Only Hex format is supported. Given value " + key + " is not valid Hex ");
        for (let i = 2; i < key.length; i++) {
            if (key[i] !== '0')
                return '0x' + key.substr(i);
        }
    }
    return '0x0';
}

function splitSignature(sig, data, hashFirst = 1) {
    sig = toHex(sig)
    const v = parseInt('0x' + sig.substr(130, 2))
    return {
        r: sig.substr(0, 66),
        s: '0x' + sig.substr(66, 64),
        v: v < 27 ? (v % 2) + 27 : v,
        recoveryParam: v < 27 ? v : v - 27,
        message: hashFirst ? data : null,
        messageHash: toHex(hashFirst ? keccak(data) : data),
        signature: toHex(sig)
    }
}

/** padStart for legacy */
function padStart(val, minLength, fill = ' ') {
    while (val.length < minLength)
        val = fill + val;
    return val;
}
/** padEnd for legacy */
function padEnd(val, minLength, fill = ' ') {
    while (val.length < minLength)
        val = val + fill;
    return val;
}
function hexchar(x) {
    const b = String.fromCharCode(x)
    let d = ''
    for (let i = 0; i < b.length; i++) {
        let h = b[i].charCodeAt(0).toString(16)
        if (h.length == 1) d += '0'
        d += h
    }
    return d
}

function convertUTF82Hex(val) {
    let bstring = ''
    for (let i = 0; i < val.length; i++) {
        let x = val.charCodeAt(i)
        if (x >= 0xD800 && x <= 0xDBFF && i + 1 < val.length && (val.charCodeAt(i + 1) & 0xFC00) == 0xDC00)
            x = ((x & 0x3FF) << 10) + (val.charCodeAt(++i) & 0x3FF) + 0x10000
        if ((x & 0xFFFFFF80) == 0) { // 1-byte sequence
            bstring += hexchar(x);
            continue
        }
        if ((x & 0xFFFFF800) == 0)  // 2-byte sequence
            bstring += hexchar(((x >> 6) & 0x1F) | 0xC0);
        else if ((x & 0xFFFF0000) == 0) { // 3-byte sequence
            if (x >= 0xD800 && x <= 0xDFFF) throw new Error('Invalid utf val')
            bstring += hexchar(((x >> 12) & 0x0F) | 0xE0)
            bstring += hexchar(((x >> 6) & 0x3f) | 0x80)
        }
        else if ((x & 0xFFE00000) == 0) { // 4-byte sequence
            bstring += hexchar(((x >> 18) & 0x07) | 0xF0);
            bstring += hexchar(((x >> 12) & 0x3f) | 0x80)
            bstring += hexchar(((x >> 6) & 0x3f) | 0x80)
        }
        bstring += hexchar((x & 0x3F) | 0x80);
    }
    return bstring
}

function soliditySha3(...args) {
    return toHex(keccak('0x' + args.map(_ => {
        switch (typeof (_)) {
            case 'object': return { t: _.t || _.type, v: _.v || _.value }
            case 'number':
            case 'bigint':
                return { t: _ < 0 ? 'int256' : 'uint256', v: _ }
            case 'string':
                const n = parseInt(_)
                if (n && !_.startsWith('0x')) return { t: n < 0 ? 'int256' : 'uint256', v: n }
                return { t: _.substr(0, 2) == '0x' ? 'bytes' : 'string', v: _ }
            case 'boolean':
                return { t: 'bool', v: _ }
            default:
                return { t: 'bytes', v: toHex(_) }
        }
    }).map(({ t, v }) => {
        if (t == 'bool') t = 'uint8';
        if (t == 'bytes' || t == 'string') return toHex(v).substr(2)
        if (t.startsWith('bytes')) return toHex(v, parseInt(t.substr(5))).substr(2)
        else if (t.startsWith('uint')) return toHex(v, parseInt(t.substr(4)) / 8).substr(2)
        else if (t.startsWith('int')) return toHex(v, parseInt(t.substr(3)) / 8).substr(2)
        else throw new Error('Unsupported type ' + t)
    }).join('')))
}

function isAddress(ad) {
    return /^0x[0-9a-fA-F]{40}$/g.test(ad)
}

function createSignatureHash(def) {
    return keccak(def.name + createSignature(def.inputs))
}

const util = {
    checkForError,
    promisify,
    toHex,
    toNumber,
    toUtf8,
    toBigInt: v => convertBigInt(v),
    toBuffer,
    toSimpleHex,
    toMinHex,
    padStart,
    padEnd,
    keccak,
    toChecksumAddress,
    abiEncode,
    abiDecode,
    ecSign,
    splitSignature,
    private2address,
    soliditySha3,
    randomBytes,
    createSignatureHash,
    toUint8Array,
    base64Decode,
    base64Encode,
    checkAddressChecksum,
    isAddress,
    getVersion
}

// add as static proporty and as standard property.
IN3.util = util
IN3.prototype.util = util


class SimpleSigner {
    constructor(...pks) {
        this.accounts = {}
        if (pks) pks.forEach(_ => this.addAccount(_))
    }

    getAccounts() {
        return Object.keys(this.accounts)
    }

    addAccount(pk) {
        const adr = private2address(pk)
        this.accounts[adr] = toBuffer(pk)
        return adr
    }


    async canSign(address) {
        return !!this.accounts[toChecksumAddress(address)]
    }

    async sign(data, account, type, ethV = true) {
        const pk = this.accounts[toChecksumAddress(account)]
        if (!pk || pk.length != 32) throw new Error('Account not found for signing ' + account)
        return ecSign(pk, data, type, ethV)

    }

}

IN3.SimpleSigner = SimpleSigner

