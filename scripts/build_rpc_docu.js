const yaml = require('../wasm/test/node_modules/yaml')
const fs = require('fs')

let docs = {}, config = {}, types = {}
const asArray = val => val == undefined ? [] : (Array.isArray(val) ? val : [val])
const link = (name, label) => '[' + (label || name) + '](#' + name.toLowerCase().replace('_', '-') + ')'
const getType = val => typeof val === 'object' ? val : (types['' + val] || val)
function scan(dir) {
    for (const f of fs.readdirSync(dir, { withFileTypes: true })) {
        if (f.name == 'rpc.yml') {
            const ob = yaml.parse(fs.readFileSync(dir + '/' + f.name, 'utf-8'))
            if (ob.types) {
                types = { ...types, ...ob.types }
                delete ob.types
            }
            for (const k of Object.keys(ob)) {
                if (ob[k].config) config = { ...config, ...ob[k].config }
                delete ob[k].config
                docs[k] = { ...docs[k], ...ob[k] }
            }
        }
        else if (f.isDirectory()) scan(dir + '/' + f.name)
    }
}
function print_object(def, pad, useNum) {
    let i = 1
    for (const prop of Object.keys(def)) {
        let s = pad + (useNum ? ((i++) + '.') : '*') + ' **' + prop + '**'
        const p = def[prop]
        const pt = getType(p.type)
        if (p.type) s += ' : `' + (typeof p.type === 'string' ? p.type : 'object') + '`'
        if (p.optional) s += ' *(optional)*'
        if (p.descr) s += ' - ' + p.descr
        if (p.default) s += ' (default: `' + JSON.stringify(p.default) + '`)'
        if (p.alias) s += '\n' + pad + 'The data structure of ' + prop + ' is the same  as ' + link(p.alias) + '. See Details there.'
        console.log(s)
        if (typeof pt === 'object') {
            console.log('The ' + prop + ' object supports the following properties :\n' + pad)
            print_object(pt, pad + '    ')
        }
        if (p.example) console.log('\n' + pad + '    *Example* : ' + prop + ': ' + JSON.stringify(p.example))
        console.log(pad + '\n')
    }
}

scan('../c/src')
docs.in3.in3_config.params.config.type = config
console.log('# API RPC\n\n')
console.log('This section describes the behavior for each RPC-method supported with incubed.\n\nThe core of incubed is to execute rpc-requests which will be send to the incubed nodes and verified. This means the available RPC-Requests are defined by the clients itself.\n\n')

for (const s of Object.keys(docs).sort()) {
    const rpcs = docs[s]
    console.log("## " + s + "\n\n")
    if (rpcs.descr) console.log(rpcs.descr + '\n')
    delete rpcs.descr
    for (const rpc of Object.keys(rpcs).sort()) {
        const def = rpcs[rpc]
        console.log('### ' + rpc + '\n\n')
        asArray(def.alias).forEach(_ => console.log(rpc + ' is just an alias for ' + link(_) + '.See Details there.\n\n'))
        if (def.descr) console.log(def.descr + '\n')
        if (def.params) {
            console.log("*Parameters:*\n")
            print_object(def.params, '', true)
            console.log()
        }
        else if (!def.alias)
            console.log("*Parameters:* - \n")
        if (def.in3Params) {
            console.log('The following in3-configuration will have an impact on the result:\n\n');
            print_object(getType(def.in3Params), '')
            console.log()
        }

        if (def.returns) {
            if (def.returns.type) {
                console.log('*Returns:* ' + (typeof def.returns.type === 'string' ? ('`' + def.returns.type + '`') : '`object`') + '\n\n' + def.returns.descr + '\n')
                const pt = getType(def.returns.type)
                if (typeof pt === 'object') {
                    console.log('\nThe return value contains the following properties :\n')
                    print_object(pt, '')
                }
            }
            else if (def.returns.alias)
                console.log('*Returns:*\n\nThe Result of `' + rpc + '` is the same as ' + link(def.returns.alias) + '. See Details there.\n')
            else
                console.log('*Returns:*\n\n' + (def.returns.descr || '') + '\n')
        }

        if (def.proof) {
            console.log('*Proof:*\n\n' + (def.proof.descr || '') + '\n')
            const pt = getType(def.proof.type)
            if (def.proof.alias)
                console.log('The proof will be calculated as described in ' + link(def.proof.alias) + '. See Details there.\n\n')

            if (pt) {
                console.log("This proof section contains the following properties:\n\n")
                print_object(pt, '')
                console.log("\n\n")
            }
        }

        asArray(def.example).forEach(ex => {
            const req = { method: rpc, params: ex.request || [] }
            if (def.proof) req.in3 = { "verification": "proof", ...ex.in3Params }
            const data = { result: ex.response || null }
            if (ex.in3) data.in3 = ex.in3

            console.log('*Example:*\n')

            console.log('```js\n//---- Request -----\n\n' + JSON.stringify(req, null, 2))
            console.log('\n//---- Response -----\n\n' + JSON.stringify(data, null, 2))
            console.log('```\n')

            console.log('```yaml\n# ---- Request -----\n\n' + yaml.stringify(req))
            console.log('\n# ---- Response -----\n\n' + yaml.stringify(data))
            console.log('```\n')
        })
    }
}


