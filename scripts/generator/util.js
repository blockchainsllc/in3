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


exports.apiPath = function apiPath(api_name, all) {
    api_name = exports.snake_case(api_name)
    if (api_name == 'util') api_name = 'utils'
    const api = all.apis.find(_ => _.api == api_name)
    if (!api) throw new Error("No api for " + api_name)
    const aconf = all.apis.find(_ => _.api == api_name).conf
    let v = aconf.extensionVar || api_name
    if (v == 'wallet') v = 'defaultWallet'
    if (v == 'util') v = 'utils'
    if (aconf.extension)
        return apiPath(aconf.extension, all) + '.' + v
    return v
}
