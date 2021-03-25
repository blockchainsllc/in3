#!/usr/bin/env node

const fs = require('fs')
const { execSync } = require('child_process')

const exists = (a, name) => a.find(_ => _.name == name)
const grep = (pattern, start) => execSync('grep -r -E ' + pattern + ' --exclude-dir third-party --exclude "debug.h" ' + start).toString().split('\n').filter(_ => _)
const strings = (start, c) => grep(c + '[0-9a-zA-Z_]{4,}' + c, start).map(_ => ((/.*[\"']([0-9a-zA-Z_]{4,})[\"'].*/g).exec(_) || ['', ''])[1]).filter(_ => _)
const getRPCHandlers = () => grep('TRY_RPC', '../c/src').map(line => {
    const r = (/(.*?\.c).*TRY_RPC\(\"([^\"]+).*/gm).exec(line)
    return { file: r[1], name: r[1].indexOf('zksync') == -1 ? r[2] : 'zksync_' + r[2], type: 'handler' }
})
const getRPCVerifiers = () => grep('VERIFY_RPC', '../c/src').reduce((p, line) => {
    let r = (/(.*?\.c).*?VERIFY_RPC\(\"([^\"]+)(.*)/gm).exec(line)
    const file = r[1]
    if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'verifier' })
    while (r = (/(.*?VERIFY_RPC)\(\"([^\"]+)(.*)/gm).exec(r[3])) {
        if (!exists(p, r[2])) p.push({ file, name: r[2], type: 'verifier' })
    }
    return p
}, [])
const check = (val, c) => val ? ((++res[c]) && '   \u2705  ') : '   \u274c  '
const res = { doc: 0, java: 0, wasm: 0, python: 0, rust: 0, dotnet: 0, c: 0, autocompl: 0 }
const doc_rpc = grep('\"^### \"', '../../../doc/docs/rpc.md').map(_ => _.substring(_.indexOf('# ') + 2).trim())
const java_rpc = strings('../java/src', '"')
const wasm_rpc = strings('../wasm/src', '\'')
const python_rpc = strings('../python/in3', '"')
const rust_rpc = strings('../rust/in3-rs/src', '"')
const dotnet_rpc = strings('../dotnet/In3', '"', '*.cs')
const c_api = strings('../c/src/api', '"',)
const autocomplete = grep("\"'.*?:\"", '_in3.sh').map(_ => ((/'([a-zA-Z0-9_]+):/gm).exec(_) || ["", ""])[1])
const all_rpc_names = [...getRPCHandlers(), ...getRPCVerifiers()].map(_ => _.name).filter((v, i, a) => a.indexOf(v) === i)
doc_rpc.filter(_ => all_rpc_names.indexOf(_) == -1).forEach(_ => all_rpc_names.push(_ + '*'))
all_rpc_names.sort()
console.log('RPC-Method'.padEnd(40) + '     doc   wasm   java   python  rust dotnet  c_api autocmpl')
console.log('-'.padEnd(44 + 7 * 8, '-'))


all_rpc_names.forEach(rpc =>
    console.log(rpc.padEnd(40) + ' : '
        + check(doc_rpc.indexOf((rpc = rpc.replace('*', ''))) != -1, 'doc')
        + check(wasm_rpc.indexOf(rpc) != -1, 'wasm')
        + check(java_rpc.indexOf(rpc) != -1, 'java')
        + check(python_rpc.indexOf(rpc) != -1, 'python')
        + check(rust_rpc.indexOf(rpc) != -1, 'rust')
        + check(dotnet_rpc.indexOf(rpc) != -1, 'dotnet')
        + check(c_api.indexOf(rpc) != -1, 'c')
        + check(autocomplete.indexOf(rpc) != -1, 'autocompl')
    )
)
console.log("\nSummary:")
Object.keys(res).forEach(k => console.log(k.padEnd(8) + ': ' + (res[k] * 100 / all_rpc_names.length).toFixed(0) + ' % '))

