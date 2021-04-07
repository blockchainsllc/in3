const camelCase = n => n.split('_').map(_ => _.substr(0, 1).toUpperCase() + _.substr(1)).join('')

exports.camelCase = camelCase
exports.camelCaseUp = s => {
    if (!s) return ''
    if (s[s.length - 1] == '.') s = s.substr(0, s.length - 1)
    s = s.substr(s.lastIndexOf('.') + 1)
    return s.substr(0, 1).toUpperCase() + camelCase(s).substr(1)
}
exports.camelCaseLow = s => s ? s.substr(0, 1).toLowerCase() + camelCase(s).substr(1) : ''

exports.asArray = val => val == undefined ? [] : (Array.isArray(val) ? val : [val])
exports.link = (name, label) => '[' + (label || name) + '](#' + name.toLowerCase().replace('_', '-') + ')'
exports.getType = (val, types) => typeof val === 'object' ? val : (types['' + val] || val)
exports.toCmdParam = val => (typeof val == 'object' || Array.isArray(val) || ('' + val).indexOf(' ') >= 0) ? "'" + JSON.stringify(val) + "'" : ('' + val)
exports.short_descr = function (d) {
    let zd = (d || '').trim()
    if (zd.indexOf('.') >= 0) zd = zd.substr(0, zd.indexOf('.'))
    if (zd.indexOf('\n') >= 0) zd = zd.substr(0, zd.indexOf('\n'))
    if (zd.indexOf('[') >= 0) zd = zd.substr(0, zd.indexOf('['))
    if (zd.length > 100) zd = zd.substr(0, 100) + '...'
    return zd
}