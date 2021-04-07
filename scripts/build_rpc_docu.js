const yaml = require('../wasm/test/node_modules/yaml')
const fs = require('fs')
const swift = require('./generator/swift')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    link,
    toCmdParam,
    short_descr
} = require('./generator/util')

const doc_dir = process.argv[process.argv.length - 1]
const main_conf = yaml.parse(fs.readFileSync('../c/src/cmd/in3/in3.yml', 'utf-8'))

const rpc_doc = []
const config_doc = []
const main_help = []
const main_aliases = []
const bool_props = []

let docs = {}, config = {}, types = {}


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



function print_object(def, pad, useNum, doc) {
    let i = 1
    for (const prop of Object.keys(def)) {
        let s = pad + (useNum ? ((i++) + '.') : '*') + ' **' + prop + '**'
        const p = def[prop]
        const pt = getType(p.type, types)
        if (p.type) s += ' : `' + (typeof p.type === 'string' ? p.type : 'object') + '`'
        if (p.optional) s += ' *(optional)*'
        if (p.descr) s += ' - ' + p.descr
        if (p.default) s += ' (default: `' + JSON.stringify(p.default) + '`)'
        if (p.enum) s += '\n' + pad + 'Possible Values are:\n\n' + Object.keys(p.enum).map(v => pad + '    - `' + v + '` : ' + p.enum[v]).join('\n') + '\n'
        if (p.alias) s += '\n' + pad + 'The data structure of ' + prop + ' is the same  as ' + link(p.alias) + '. See Details there.'
        doc.push(s)
        if (typeof pt === 'object') {
            rpc_doc.push('The ' + prop + ' object supports the following properties :\n' + pad)
            print_object(pt, pad + '    ', false, doc)
        }
        if (p.example) rpc_doc.push('\n' + pad + '    *Example* : ' + prop + ': ' + JSON.stringify(p.example))
        doc.push(pad + '\n')
    }
}


function handle_config(conf, pre, title, descr) {
    if (title) config_doc.push('\n## ' + title + '\n')
    for (const key of Object.keys(conf)) {
        const c = conf[key]
        // handle bindings

        swift.updateConfig(pre, c, key)

        // handle doc
        if (!pre) {
            let s = '\n' + (title ? '#' : '') + '## ' + key + '\n\n' + c.descr
            if (c.optional) s += ' *This config is optional.*'
            if (c.default) s += ' (default: `' + JSON.stringify(c.default) + '`)'
            if (c.type) s += '\n\n Type: `' + (typeof c.type === 'string' ? c.type : 'object') + '`'
            if (c.enum) s += '\n\nPossible Values are:\n\n' + Object.keys(c.enum).map(v => '- `' + v + '` : ' + c.enum[v]).join('\n') + '\n'
            config_doc.push(s)
            if (typeof (c.type) === 'object') {
                config_doc.push('The ' + key + ' object supports the following properties :\n')
                print_object(c.type, '', false, config_doc)
            }
            if (c.example !== undefined) {
                config_doc.push('\n*Example:*\n')
                asArray(c.example).forEach(ex => {
                    config_doc.push('```sh')
                    if (typeof (ex) == 'object')
                        config_doc.push('> in3 ' + Object.keys(ex).filter(_ => typeof (ex[_]) !== 'object').map(k => '--' + pre + key + '.' + k + '=' + ex[k]).join(' ') + '  ....\n')
                    else
                        config_doc.push([...asArray(c.cmd).map(_ => '-' + _), '--' + pre + key].map(_ => '> in3 ' + _ + (ex === true ? '' : (_.startsWith('--') ? '=' : ' ') + ex) + '  ....').join('\n') + '\n')
                    config_doc.push('```\n')
                    if (!title)
                        config_doc.push('```js\nconst in3 = new IN3(' + JSON.stringify({ [key]: ex }, null, 2) + ')\n```\n')
                })

            }
        }
        asArray(c.cmd).forEach(_ => main_aliases.push('    "' + _ + '", "' + (c.alias || (pre + key + (c.type == 'bool' ? '=true' : ''))) + '",'));
        if (c.type == 'bool') bool_props.push(pre + key);
        main_help.push(('--' + pre + key).padEnd(30) + (c.cmd ? ('-' + c.cmd) : '').padEnd(7) + short_descr(c.descr))
        let s = ''
        if (c.descr) s += '[' + short_descr(c.descr) + ']'
        if (c.type != 'bool')
            s += ':' + key + ':(' + (c.enum ? Object.keys(c.enum).join(' ') : '') + ')'
        if (typeof (c.type) === 'object')
            handle_config(c.type, pre + key + '.')
        else {
            zsh_conf.push("'--" + pre + key + (c.type != 'bool' ? '=' : '') + s + "'")
            asArray(c.cmd).forEach(_ => zsh_conf.push("'-" + _ + s + "'"))
        }
    }
}

scan('../c/src')
docs.in3.in3_config.params.config.type = config
rpc_doc.push('# API RPC\n\n')
rpc_doc.push('This section describes the behavior for each RPC-method supported with incubed.\n\nThe core of incubed is to execute rpc-requests which will be send to the incubed nodes and verified. This means the available RPC-Requests are defined by the clients itself.\n\n')
config_doc.push('# Configuration\n\n')
config_doc.push('When creating a new Incubed Instance you can configure it. The Configuration depends on the registered plugins. This page describes the available configuration parameters.\n\n')
const zsh_complete = fs.readFileSync('_in3.template', 'utf8')
let zsh_cmds = [], zsh_conf = []
for (const s of Object.keys(docs).sort()) {
    const rpcs = docs[s]
    const rdescr = rpcs.descr

    rpc_doc.push("## " + s + "\n\n")
    if (rdescr) rpc_doc.push(rdescr + '\n')
    delete rpcs.descr

    swift.generateAPI(s, rpcs, rdescr, types)

    for (const rpc of Object.keys(rpcs).sort()) {
        const def = rpcs[rpc]
        def.returns = def.returns || def.result
        def.result = def.returns || def.result
        let z = "    '" + rpc + ': ' + short_descr((def.descr || (def.alias && rpcs[def.alias].descr) || ''))

        rpc_doc.push('### ' + rpc + '\n\n')
        asArray(def.alias).forEach(_ => rpc_doc.push(rpc + ' is just an alias for ' + link(_) + '.See Details there.\n\n'))
        if (def.descr)
            rpc_doc.push(def.descr + '\n')
        if (def.params) {
            rpc_doc.push("*Parameters:*\n")
            print_object(def.params, '', true, rpc_doc)
            rpc_doc.push()
            z += ' ' + Object.keys(def.params).map(_ => '<' + _ + '>').join(' ')
        }
        else if (!def.alias)
            rpc_doc.push("*Parameters:* - \n")
        if (def.in3Params) {
            rpc_doc.push('The following in3-configuration will have an impact on the result:\n\n');
            print_object(getType(def.in3Params, types), '', false, rpc_doc)
            rpc_doc.push()
        }
        if (def.validation) rpc_doc.push('\n' + def.validation + '\n')

        if (def.returns) {
            if (def.returns.type) {
                rpc_doc.push('*Returns:* ' + (typeof def.returns.type === 'string' ? ('`' + def.returns.type + '`') : '`object`') + '\n\n' + def.returns.descr + '\n')
                const pt = getType(def.returns.type, types)
                if (typeof pt === 'object') {
                    rpc_doc.push('\nThe return value contains the following properties :\n')
                    print_object(pt, '', false, rpc_doc)
                }
            }
            else if (def.returns.alias)
                rpc_doc.push('*Returns:*\n\nThe Result of `' + rpc + '` is the same as ' + link(def.returns.alias) + '. See Details there.\n')
            else
                rpc_doc.push('*Returns:*\n\n' + (def.returns.descr || '') + '\n')
        }

        if (def.proof) {
            rpc_doc.push('*Proof:*\n\n' + (def.proof.descr || '') + '\n')
            const pt = getType(def.proof.type, types)
            if (def.proof.alias)
                rpc_doc.push('The proof will be calculated as described in ' + link(def.proof.alias) + '. See Details there.\n\n')

            if (pt) {
                rpc_doc.push("This proof section contains the following properties:\n\n")
                print_object(pt, '', false, rpc_doc)
                rpc_doc.push("\n\n")
            }
        }

        asArray(def.example).forEach(ex => {
            const req = { method: rpc, params: ex.request || [] }
            if (def.proof) req.in3 = { "verification": "proof", ...ex.in3Params }
            const data = { result: ex.response || null }
            const is_json = (typeof data.result == 'object' || Array.isArray(data.result))
            if (ex.in3) data.in3 = ex.in3

            rpc_doc.push('*Example:*\n')
            if (ex.descr) rpc_doc.push('\n' + ex.descr + '\n')

            /*
                        rpc_doc.push('```yaml\n# ---- Request -----\n\n' + yaml.stringify(req))
                        rpc_doc.push('\n# ---- Response -----\n\n' + yaml.stringify(data))
                        rpc_doc.push('```\n')
            */
            rpc_doc.push('```sh\n> in3 ' + (ex.cmdParams ? (ex.cmdParams + ' ') : '') + req.method + ' ' + (req.params.map(toCmdParam).join(' ').trim()) + (is_json ? ' | jq' : ''))
            rpc_doc.push(is_json ? JSON.stringify(data.result, null, 2) : '' + data.result)
            rpc_doc.push('```\n')

            rpc_doc.push('```js\n//---- Request -----\n\n' + JSON.stringify(req, null, 2))
            rpc_doc.push('\n//---- Response -----\n\n' + JSON.stringify(data, null, 2))
            rpc_doc.push('```\n')

        })
        z += "'"
        zsh_cmds.push(z)
    }
}

handle_config(config, '')

swift.generate_config()


handle_config(main_conf.config, '', 'cmdline options\n\nThose special options are used in the comandline client to pass additional options.\n')
main_help.push('')
main_help.push('In addition to the documented rpc-methods, those methods are also supported:')
main_help.push('')
Object.keys(main_conf.rpc).forEach(k => {
    (k + ' ' + main_conf.rpc[k]).split("\n").map(_ => _.trim()).map((_, i) => i ? '   ' + _ : _)
        .forEach(l => main_help.push(l))
})

fs.writeFileSync('_in3.sh', zsh_complete.replace('$CMDS', zsh_cmds.join('\n')).replace('$CONFS', zsh_conf.join('\n')), { encoding: 'utf8' })
fs.writeFileSync(doc_dir + '/rpc.md', rpc_doc.join('\n') + '\n', { encoding: 'utf8' })
fs.writeFileSync(doc_dir + '/config.md', config_doc.join('\n') + '\n', { encoding: 'utf8' })
fs.writeFileSync('../c/src/cmd/in3/args.h', '// This is a generated file, please don\'t edit it manually!\n\n#include <stdlib.h>\n\nconst char* bool_props[] = {' + bool_props.map(_ => '"' + _ + '", ').join('') + 'NULL};\n\nconst char* help_args = "\\\n' + main_help.map(_ => _ + '\\n').join('\\\n') + '";\n\nconst char* aliases[] = {\n' + main_aliases.join('\n') + '\n    NULL};\n', { encoding: 'utf8' })

