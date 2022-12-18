#!/usr/bin/env node

const fs = require('fs')
const { execSync } = require('child_process')

const exists = (a, name) => a.find(_ => _.name == name)
const grep = (pattern, start) => execSync('grep -r -E ' + pattern + ' --exclude-dir third-party --exclude "debug.h" ' + start).toString().split('\n').filter(_ => _)
const strings = (start, c) => grep(c + '[0-9a-zA-Z_]{4,}' + c, start).map(_ => ((/.*[\"']([0-9a-zA-Z_]{4,})[\"'].*/g).exec(_) || ['', ''])[1]).filter(_ => _)
const getRPCHandlers = () => grep('TRY_RPC', '../src').map(line => {
    const r = (/(.*?\.c).*TRY_RPC\(\"([^\"]+).*/gm).exec(line)
    return { file: r[1], name: r[1].indexOf('zksync') == -1 ? r[2] : 'zksync_' + r[2], type: 'handler' }
})
const getRPCVerifiers = () => grep('VERIFY_RPC', '../src').reduce((p, line) => {
    let r = (/(.*?\.c).*?VERIFY_RPC\(\"([^\"]+)(.*)/gm).exec(line)
    const file = r[1]
    if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'verifier' })
    while (r = (/(.*?VERIFY_RPC)\(\"([^\"]+)(.*)/gm).exec(r[3])) {
        if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'verifier' })
    }
    return p
}, [])
const getConfigs = () => grep('CONFIG_KEY', '../src').reduce((p, line) => {
    let r = (/(.*?\.c).*?CONFIG_KEY\(\"([^\"]+)(.*)/gm).exec(line)
    const file = r[1]
    if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'config' })
    while (r = (/(.*?CONFIG_KEY)\(\"([^\"]+)(.*)/gm).exec(r[3])) {
        if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'config' })
    }
    return p
}, [])
const lastToken = l => l.substring(l.lastIndexOf(' ') + 1);
const label = (n, l) => l ? ('\033[' + l + 'm' + n.replace('*', '') + '\033[0m') : n
const is_allowed = ['btc_proofTarget']
const check = (list, name, c) => list.indexOf(name) != -1 ? ((++res[c]) && '   \u2705  ') : ((list.length && is_allowed.indexOf(name) == -1) ? '   \u274c  ' : ((++res[c]) && '   \u274e  '))
const bindings = {
    doc: grep('\"^### \"', '../../../doc/docs/rpc.md').map(_ => _.substring(_.indexOf('# ') + 2).trim()),
    java: strings('../java/src', '"'),
    wasm: strings('../wasm/src', '\''),
    python: strings('../python/in3', '"'),
    rust: strings('../rust/in3-rs/src', '"'),
    dotnet: strings('../dotnet/In3', '"', '*.cs'),
    swift: strings('../swift/Sources', '"'),
    c_api: strings('../src/api', '"',),
    //    test: [...strings('../test/testdata/requests', '"'), ...strings('../test/testdata/api', '"'), ...strings('../test/testdata/cmd', ' ')],
    test: [
        ...strings('../test/testdata/requests', '"'),
        ...strings('../test/testdata/api', '"'),
        ...grep('\"^.*_.*\"', '../test/testdata/cmd').map(_ => ((/.* ([0-9a-zA-Z]+_[0-9a-zA-Z_]+).*/g).exec(_) || ['', ''])[1]).filter(_ => _)],
    autocmpl: grep("\"'.*?:\"", '_in3.sh').map(_ => ((/'([a-zA-Z0-9_]+):/gm).exec(_) || ["", ""])[1]),
}
const res = Object.keys(bindings).reduce((p, c) => ({ ...p, [c]: 0 }), {})
const all_rpc_names = [...getRPCHandlers(), ...getRPCVerifiers()].map(_ => _.name).filter((v, i, a) => a.indexOf(v) === i)
const configs = getConfigs().map(_ => _.name).filter((v, i, a) => a.indexOf(v) === i).sort()
bindings.doc.filter(_ => all_rpc_names.indexOf(_) == -1).forEach(_ => all_rpc_names.push(_ + '*'))
all_rpc_names.sort()
console.log('RPC-Method'.padEnd(40) + '    ' + Object.keys(bindings).map(_ => _.padEnd(7)).join(''))
console.log('-'.padEnd(45 + 7 * Object.keys(bindings).length, '-'))


all_rpc_names.forEach(rpc => {
    let s = '', c = 0, l = rpc.replace('*', '')
    Object.keys(bindings).forEach(k => {
        c -= res[k]
        s += check(bindings[k], l, k)
        c += res[k]
    })
    console.log(label(l.padEnd(40), rpc.endsWith('*') ? '31' : (Object.keys(bindings).length == c ? '32' : '33')) + ' : ' + s)
})

console.log('\nConfig'.padEnd(40) + '    ' + Object.keys(bindings).map(_ => _.padEnd(7)).join(''))
console.log('-'.padEnd(45 + 7 * Object.keys(bindings).length, '-'))
bindings.doc = grep('\"\\*\\*[a-zA-Z0-9_]+\\*\\*\"', '../../../doc/docs/rpc.md').map(_ => _.substring(_.indexOf('**') + 2, _.lastIndexOf('**')).trim())
bindings.wasm = grep('\"^[ ]*[a-zA-Z0-9_]+[\\?]*:.*\"', '../wasm/src').map(_ => lastToken(_.substring(0, _.lastIndexOf(':')).replace('?', '').trim()))
bindings.autocmpl = []
bindings.c_api = []
bindings.test = []
bindings.python = grep("\"self\\.[a-zA-Z0-9_]+\"", '../python/in3/model.py').map(_ => ((/self.([a-zA-Z0-9_]+)/gm).exec(_) || ["", ""])[1])
bindings.swift = grep("\"var [a-zA-Z0-9_]+\"", '../swift/Sources/In3/Config.swift').map(_ => ((/var ([a-zA-Z0-9_]+)/gm).exec(_) || ["", ""])[1])


for (const conf of configs) {
    let s = '', c = 0
    Object.keys(bindings).forEach(k => {
        c -= res[k]
        s += check(bindings[k], conf, k)
        c += res[k]
    })
    console.log(label(conf.padEnd(40), (Object.keys(bindings).length == c ? '32' : '33')) + ' : ' + s)
}



console.log("\nSummary:")
Object.keys(res).forEach(k => console.log(k.padEnd(8) + ': ' + (res[k] * 100 / (all_rpc_names.length + configs.length)).toFixed(0) + ' % '))

