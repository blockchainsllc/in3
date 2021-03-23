const yaml = require('../wasm/test/node_modules/yaml')
const fs = require('fs')

const docs = {}
let config = {}
function scan(dir) {
    for (const f of fs.readdirSync(dir, { withFileTypes: true })) {
        if (f.name == 'rpc.yml') {
            const ob = yaml.parse(fs.readFileSync(dir + '/' + f.name, 'utf-8'))
            for (const k of Object.keys(ob)) {
                if (ob[k].config) config = { ...config, ...ob[k].config }
                delete ob[k].config
                docs[k] = { ...docs[k], ...ob[k] }
            }
        }
        else if (f.isDirectory()) scan(dir + '/' + f.name)
    }
}
function print_object(def, pad) {
    for (const prop of Object.keys(def)) {
        let s = pad + '* **' + prop + '**'
        const p = def[prop]
        if (p.type) s += ' : `' + (typeof p.type === 'string' ? p.type : 'object') + '`'
        if (p.optional) s += ' *(optional)*'
        if (p.descr) s += ' - ' + p.descr
        if (p.default) s += ' (default: `' + JSON.stringify(p.default) + '`)'
        console.log(s)
        if (typeof p.type === 'object') {
            console.log('The ' + prop + ' object supports the following properties :\n' + pad)
            print_object(p.type, pad + '    ')
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
        console.log('### ' + rpc + '\n\n')
        const def = rpcs[rpc]
        if (def.descr) console.log(def.descr + '\n')
        if (def.params) {
            console.log("Parameters:\n")
            let i = 1
            for (const par of Object.keys(def.params)) {
                const p = def.params[par]
                let s = (i++) + '. `' + par + '`'
                if (p.type) s += ':' + (typeof p.type === 'string' ? p.type : 'object')
                if (p.descr) s += ' - ' + p.descr
                console.log(s)
                if (typeof p.type === 'object') {
                    console.log('\nThe ' + par + ' params support the following properties :\n')
                    print_object(p.type, '')
                }

            }
            console.log()
        }
        else
            console.log("Parameters: - \n")

        if (def.returns) {
            if (def.returns.type) {
                console.log('Returns: ' + (typeof def.returns.type === 'string' ? def.returns.type : 'object') + '\n\n' + def.returns.descr + '\n')
                if (typeof def.returns.type === 'object') {
                    console.log('\nThe return value contains the following properties :\n')
                    print_object(def.returns.type, '')
                }
            }
            else
                console.log('Returns:\n\n' + def.returns.descr + '\n')
        }

        if (def.proof) {
            console.log('Proof:\n\n' + def.proof.descr + '\n')
            if (def.proof.type) {
                console.log("This proof section contains the following properties:\n\n")
                print_object(def.proof.type, '')
                console.log("\n\n")
            }
        }



        if (def.example && def.example.request) {
            console.log('Request:\n')
            console.log('```js\n' + JSON.stringify({ method: rpc, params: def.example.request }, null, 2))
            console.log('```\n')
        }
        if (def.example && def.example.response) {
            const data = { result: def.example.response }
            if (def.example.in3) data.in3 = def.example.in3
            console.log('Response:\n')
            console.log('```js\n' + JSON.stringify(data, null, 2))
            console.log('```\n')
        }

    }
}


