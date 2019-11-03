const fixLength = (hex) => hex.length % 2 ? '0' + hex : hex
function call_string(name, ...params_values) {
    const res = in3w.ccall(name, 'number', params_values.map(_ => _ && _.__proto__ === Uint8Array.prototype ? 'array' : typeof _), params_values)
    if (!res) return null
    const result = UTF8ToString(res)
    if (result && result.startsWith(":ERROR:")) throw new Error(result.substr(7))
    _free(res)
    return result
}
function call_buffer(name, len, ...params_values) {
    const res = in3w.ccall(name, 'number', params_values.map(_ => _ && _.__proto__ === Uint8Array.prototype ? 'array' : typeof _), params_values)
    if (!res) return null
    const result = HEAPU8.slice(res, res + retType)
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

function toUtf8(val) {
    if (!val) return val
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
    val = toBuffer(val)
    return call_buffer('keccak', 32, val, val.byteLength)
}
function toChecksumAddress(val, chainId = 0) {
    if (!val) return val
    return call_string('to_checksum_address', toBuffer(val, 20), chainId);
}



function abiEncode(sig, ...params) {
    function convert(a) {
        if (Array.isArray(a)) return a.map(convert)
        return (typeof a === 'bigint' || typeof a === 'object') ? toHex(a) : a
    }

    return call_string('abi_encode', sig, JSON.stringify(convert(params)))
}

/**
 * converts any value as hex-string
 */
function toHex(val, bytes) {
    if (val === undefined)
        return undefined;
    let hex = ''
    if (typeof val === 'string')
        hex = val.startsWith('0x')
            ? val.substr(2)
            : (parseInt(val[0])
                ? BigInt(val).toString(16)
                : Object.keys(val).map(_ => padStart(_.charCodeAt(0).toString(16), 2, '0')).join('')
            )
    else if (typeof val === 'number' || typeof val === 'bigint')
        hex = val.toString(16)
    else if (val && val._isBigNumber) // BigNumber
        hex = val.toHexString()
    else if (val && (val.redIMul || val.readBigInt64BE)) // bn.js or nodejs Buffer
        hex = val.toString('hex')
    else if (val && val.byteLength) {
        const ar = val.buffer ? val : new Uint8Array(val)
        for (let i = 0; i < val.byteLength; i++) hex += padStart(ar[i].toString(16), 2, '0')
    }
    else
        throw new Error('Unknown or unsupported type : ' + JSON.stringify(val))

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
        case 'bigint':
        case 'string':
            return parseInt(val)
        case 'undefined':
        case 'null':
            return 0
        default:
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
/**
 * converts any value as Buffer
 *  if len === 0 it will return an empty Buffer if the value is 0 or '0x00', since this is the way rlpencode works wit 0-values.
 */
function toBuffer(val, len = -1) {
    if (val && val._isBigNumber)
        val = val.toHexString();
    if (typeof val == 'string') {
        let b;
        if (val && val.length && (parseInt(val) || val == '0'))
            val = '0x' + BigInt(val).toString(16)

        if (val.startsWith('0x')) {
            val = fixLength(val.substr(2))
            b = new Uint8Array(val.length >> 1)
            for (let i = 0; i < b.byteLength; i++) b[i] = parseInt('0x' + val.substr(i << 1, 2))
        }
        else {
            b = new Uint8Array(val.length)
            for (let i = 0; i < b.byteLength; i++) b[i] = val.charCodeAt(i)
        }
        val = b
    }
    else if (typeof val == 'number')
        val = val === 0 && len === 0 ? new Uint8Array(0) : toBuffer(fixLength(val.toString(16)), len);
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
 * returns a address from a private key
 */
function getAddress(pk) {
    const key = toBuffer(pk);
    return ethUtil.toChecksumAddress(ethUtil.privateToAddress(key).toString('hex'));
}
/** removes all leading 0 in the hexstring */
function toMinHex(key) {
    if (typeof key !== 'string')
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

const util = {
    checkForError,
    promisify,
    toHex,
    toNumber,
    toUtf8,
    checkForError,
    toBigInt,
    toBuffer,
    toSimpleHex,
    toMinHex,
    padStart,
    padEnd,
    keccak,
    toChecksumAddress,
    abiEncode
}

// add as static proporty and as standard property.
IN3.util = util
IN3.prototype.util = util

