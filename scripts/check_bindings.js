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
const check = val => val ? '   \u2705  ' : '   \u274c  '

const doc_rpc = grep('\"### \"', '../../../doc/docs/rpc.md').map(_ => _.substring(_.indexOf('# ') + 2).trim())
const java_rpc = strings('../java/src', '"')
const wasm_rpc = strings('../wasm/src', '\'')
const python_rpc = strings('../python/in3', '"')
const rust_rpc = strings('../rust/in3-rs/src', '"')
const dotnet_rpc = strings('../dotnet/In3', '"', '*.cs')
const all_rpc_names = [...getRPCHandlers(), ...getRPCVerifiers()].map(_ => _.name).filter((v, i, a) => a.indexOf(v) === i)
all_rpc_names.sort()
console.log('RPC-Method'.padEnd(40) + '     doc   java   wasm   python  rust dotnet')
console.log('-'.padEnd(42 + 7 * 6, '-'))


all_rpc_names.forEach(rpc =>
    console.log(rpc.padEnd(40) + ' : '
        + check(doc_rpc.indexOf(rpc) != -1)
        + check(java_rpc.indexOf(rpc) != -1)
        + check(wasm_rpc.indexOf(rpc) != -1)
        + check(python_rpc.indexOf(rpc) != -1)
        + check(rust_rpc.indexOf(rpc) != -1)
        + check(dotnet_rpc.indexOf(rpc) != -1)
    )
)


