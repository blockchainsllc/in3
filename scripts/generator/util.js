const camelCase = n => n.split('_').map(_ => _.substr(0, 1).toUpperCase() + _.substr(1)).join('')

exports.camelCase = camelCase
exports.camelCaseUp = s => {
    if (!s) return ''
    if (s[s.length - 1] == '.') s = s.substr(0, s.length - 1)
    s = s.substr(s.lastIndexOf('.') + 1)
    return s.substr(0, 1).toUpperCase() + camelCase(s).substr(1)
}
exports.camelCaseLow = s => s ? s.substr(0, 1).toLowerCase() + camelCase(s).substr(1) : ''
exports.snake_case = s => {
    let r = s[0].toLowerCase()
    for (let i = 1; i < s.length; i++) {
        if (s[i].toUpperCase() == s[i] && s[i - 1].toUpperCase() != s[i - 1]) r += '_'

        r += s[i] == '-' ? '_' : s[i].toLowerCase()
    }
    return r.split('__').join('_')
}
exports.mergeTo = function (src, dst) {
    Object.keys(src).forEach(k => {
        if (Array.isArray(src[k]) || typeof (dst[k]) !== 'object')
            dst[k] = src[k]
        else exports.mergeTo(src[k], dst[k])
    })
}
exports.asArray = val => val == undefined ? [] : (Array.isArray(val) ? val : [val])
exports.link = (name, label) => '[' + (label || name) + '](#' + name.toLowerCase().replace('_', '-') + ')'
exports.getType = (val, types) => {
    if (typeof val === 'object') {
        if (val._extends) {
            const base = exports.getType(val._extends, types)
            delete val._extends
            Object.assign(val, { ...base, ...val })
        }
        return val
    }
    if (!val) return undefined
    return exports.getType(types['' + val], types) || val
}
exports.toCmdParam = val => (typeof val == 'object' || Array.isArray(val) || ('' + val).indexOf(' ') >= 0) ? "'" + JSON.stringify(val) + "'" : ('' + val)
exports.short_descr = function (d) {
    let zd = (d || '').trim()
    if (zd.indexOf('.') >= 0) zd = zd.substr(0, zd.indexOf('.'))
    if (zd.indexOf('\n') >= 0) zd = zd.substr(0, zd.indexOf('\n'))
    if (zd.indexOf('[') >= 0) zd = zd.substr(0, zd.indexOf('['))
    if (zd.length > 100) zd = zd.substr(0, 100) + '...'
    return zd.split("'").join(' ')
}
exports.addAll = function addAll(array, elements) {
    exports.asArray(elements).forEach(_ => array.push(_))
}
exports.typeName = (def, code) => (code ? '`' : '') + ((def.key ? '{key:$t}' : (def.array ? '$t[]' : "$t")) + (def.optional ? '?' : '')).replace('$t', typeof (def.type) === 'string' ? def.type : 'object') + (code ? '`' : '')


exports.apiPath = function apiPath(api_name, all) {
    api_name = exports.snake_case(api_name)
    if (api_name == 'util') api_name = 'utils'
    const api = all.apis.find(_ => exports.snake_case(_.api) == api_name)
    if (!api) throw new Error("No api for " + api_name)
    const aconf = all.apis.find(_ => exports.snake_case(_.api) == api_name).conf
    let v = aconf.extensionVar || api_name
    if (v == 'wallet') v = 'defaultWallet'
    if (v == 'util') v = 'utils'
    if (aconf.extension == 'contracts') v = 'get' + exports.camelCaseUp(v) + '(contractAddress)'
    if (aconf.extension)
        return apiPath(aconf.extension, all) + '.' + v
    return v
}

exports.create_example_arg = function create_example_arg(argname, def, types) {
    if (def.example) return def.example
    let val = argname
    switch (def.type) {
        case 'address':
            val = '0x1e7efabcf06dba5438c0991bc2c6612bd6e7299c'
            break
        case 'bool':
            val = true
            break
        case 'bytes':
            val = '0xfcb05a67afd9381f78b2150250'
            break
        case 'bytes32':
            val = '0xfcb05a67afd9381f78b215b49c072bacb27dd0615bcddf74ad7c12d50aef0250'
            break
        case 'uint64':
            val = 97773
            break
        case 'double':
            val = 1.045
            break
        case 'string':
            if (argname.indexOf('uuid') >= 0)
                val = '4651c4fe-0117-4504-a53c-07d34d9c1b9a'
            else if (argname.indexOf('date') >= 0 || argname.indexOf('createdAt') >= 0 || argname.indexOf('time') >= 0)
                val = '2022-04-04T05:37:47.762Z'
            else
                val = '<' + argname + '>'
            break
        case 'uint32':
            val = 23
            break
        default: {
            const t = typeof def.type == 'object' ? def.type : types[def.type]
            if (!t) break
            val = {}
            Object.keys(t).filter(_ => !t[_].optional).forEach(k => val[k] = create_example_arg(k, t[k], types))
        }
    }
    return def.array ? [val] : val
}