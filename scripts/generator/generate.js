#!/usr/bin/env node

const yaml = require('yaml')
const fs = require('fs')
const { resolve } = require('path')
const { generate_openapi } = require('./openapi')
const { generate_solidity } = require('./solidity')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    link,
    toCmdParam,
    short_descr
} = require('./util')

const cmake_types = {}
const cmake_deps = {}
const pargs = process.argv.slice(2)
const in3_core_dir = process.argv[1].replace('/scripts/generator/generate.js', '')
const src_dirs = []
const doc_dir = []
const args_file = []
const zsh_file = []
const generators = []
const rpc_dirs = {}
const cmake = {}
const zsh_cmds = []
const zsh_conf = []
const api_conf = {}

let cmdName = 'in3'
let sdkName = 'IN3'
let cmake_data = null
process.argv.slice(2).forEach(a => {
    if (a.startsWith('--src=')) src_dirs.push(a.substr(6))
    else if (a.startsWith('--doc=')) doc_dir.push(a.substr(6))
    else if (a.startsWith('--arg=')) args_file.push(a.substr(6))
    else if (a.startsWith('--zsh=')) zsh_file.push(a.substr(6))
    else if (a.startsWith('--cmd=')) cmdName = a.substr(6)
    else if (a.startsWith('--sdk=')) sdkName = a.substr(6)
    else if (a.startsWith('--gen=')) generators.push(require(a.substr(6)))
    else if (a.startsWith('--cmake=')) {
        try {
            cmake_data = JSON.parse(fs.readFileSync(a.substr(8), 'utf8'))
            cmake.modules = {}
            cmake_data.dirs.split(';').forEach(_ => rpc_dirs[resolve(_)] = true)
            cmake_data.apis.split(';').forEach(_ => cmake.modules[_] = true)
        }
        catch (x) {
            console.error(x)
        }
    }
    else if (a.startsWith('--api.')) {
        const args = a.substring(6).split('=', 2);
        if (!args.length == 2) throw new Error("Invalid api-config " + a);
        cmake[args[0]] = args[1]
    }
    else throw new Error('Invalid argument : ' + a)
})
if (!src_dirs.length) src_dirs.push('../c/src')
let examples = {};
(doc_dir || []).forEach(p => {
    try {
        examples = { ...examples, ...JSON.parse(fs.readFileSync(p + '/rpc_examples.json', 'utf-8')) }
    } catch (x) { }
})

//const doc_dir = process.argv[process.argv.length - 1]
const main_conf = yaml.parse(fs.readFileSync(in3_core_dir + '/c/src/cmd/in3/in3.yml', 'utf-8'))
const typeName = (def, code) => (code ? '`' : '') + ((def.key ? '{key:$t}' : (def.array ? '$t[]' : "$t")) + (def.optional ? '?' : '')).replace('$t', typeof (def.type) === 'string' ? def.type : 'object') + (code ? '`' : '')
const rpc_doc = []
const config_doc = []
const main_help = []
const main_aliases = []
const bool_props = []

let docs = {}, config = {}, types = {}, testCases = {}

function check_depends(n) {
    if (Array.isArray(n)) return check_depends(n[0])
    return (n && n.depends && cmake.modules)
        ? !asArray(n.depends).find(_ => !cmake.modules[_])
        : true
}
function is_testcase(t) {
    t = asArray(t)[0]
    return t.expected_output !== undefined || t.expected_failure
}

async function scan(dir) {
    const is_valid = !!(Object.keys(rpc_dirs).length == 0 || rpc_dirs[resolve(dir)] || dir.endsWith('api_ext') || dir.endsWith('core/client'))
    for (const f of fs.readdirSync(dir, { withFileTypes: true })) {
        if (f.name == 'rpc.yml' && !is_valid) console.error("SKIP" + dir + '/rpc.yml')
        if (f.name == 'rpc.yml' && is_valid) {
            console.error('parse ' + dir + '/' + f.name)
            const fullpath = resolve(dir).trim()
            const ob = yaml.parse(fs.readFileSync(dir + '/' + f.name, 'utf-8'))
            if (ob.types) {
                Object.keys(ob.types).forEach(t => cmake_types[t] = fullpath)
                types = { ...types, ...ob.types }
                delete ob.types
            }
            for (const k of Object.keys(ob)) {
                const apic = { ...api_conf[k], ...ob[k].api }
                api_conf[k] = apic
                const generate_rpc = apic.generate_rpc
                delete apic.generate_rpc
                delete ob[k].api
                if (apic.config) config = { ...config, ...apic.config }
                for (const t of Object.keys(ob[k])) {
                    if (!check_depends(ob[k][t])) {
                        delete ob[k][t]
                        console.error(`skipping ${k} :: ${t}`)
                    }
                }
                if (generate_rpc) {
                    if (generate_rpc.openapi)
                        await generate_openapi({ generate_rpc, types, api_name: k, api: ob[k], url: generate_rpc.openapi.startsWith('http') ? generate_rpc.openapi : fullpath + '/' + generate_rpc.openapi })
                    if (generate_rpc.solidity)
                        await generate_solidity({ generate_rpc, types, api_name: k, api: ob[k], dir: fullpath, api_def: apic })
                    Object.keys(ob[k]).forEach(_ => {
                        ob[k][_].src = fullpath
                        ob[k][_].generate_rpc = generate_rpc
                    })
                    cmake_types[fullpath] = true
                }
                if (!generators.length && apic.fields && lastAPI) {
                    for (const n of Object.keys(ob[k]).filter(_ => docs[lastAPI][_])) delete ob[k][n]
                    docs[lastAPI] = { ...docs[lastAPI], ...ob[k] }
                }
                else
                    docs[k] = { ...docs[k], ...ob[k] }
                lastAPI = k
            }
        }
        else if (f.name.startsWith("testCases") && f.name.endsWith('.yml') && is_valid) {
            console.error('parse ' + dir + '/' + f.name)
            const ob = yaml.parse(fs.readFileSync(dir + '/' + f.name, 'utf-8'))
            for (const k of Object.keys(ob)) {
                for (const t of Object.keys(ob[k])) {
                    if (!check_depends(ob[k][t])) {
                        delete ob[k][t]
                        console.error(`skipping ${k} :: ${t}`)
                    }
                    else if (testCases[k] && testCases[k][t] && is_testcase(testCases[k][t]))
                        ob[k][t] = [...asArray(testCases[k][t]), ...asArray(ob[k][t])]  // merge testcases
                }
                testCases[k] = { ...testCases[k], ...ob[k] }
            }
        }
        else if (f.name == 'CMakeLists.txt') {
            const content = fs.readFileSync(`${dir}/${f.name}`, 'utf8')
            content.split('add_static_library').slice(1).forEach(s => {
                const c = (s.split('(')[1] || '').split(')', 1)[0]
                if (!c) return
                const words = c.split(/[ ,\n]+/).filter(_ => _)
                let n = words.indexOf('NAME')
                let name = words[n + 1]
                let depends = []
                n = words.indexOf('DEPENDS')
                if (n >= 0) depends = words.slice(n + 1)
                cmake_deps[name] = {
                    dir: resolve(dir).trim(),
                    depends
                }
            })
        }
        else if (f.isDirectory()) await scan(dir + '/' + f.name)
    }
}

function print_object(def, pad, useNum, doc, pre) {
    let i = 1
    for (const prop of Object.keys(def)) {
        let s = pad + (useNum ? ((i++) + '.') : '*') + ' **' + prop + '**'
        const p = def[prop]
        while (typeof p.type == 'string' && typeof (types[p.type]) === 'string') p.type = types[p.type]

        const pt = getType(p.type, types)
        if (p.type) s += ' : ' + typeName(p, true)
        if (p.optional) s += ' *(optional)*'
        if (p.descr) s += ' - ' + p.descr
        if (p.key) s += ' with ' + p.key + ' as keys in the object'
        if (p.default) s += ' (default: `' + JSON.stringify(p.default) + '`)'
        if (p.enum) s += '\n' + pad + 'Possible Values are:\n\n' + Object.keys(p.enum).map(v => pad + '    - `' + v + '` : ' + p.enum[v]).join('\n') + '\n'
        if (p.alias) s += '\n' + pad + 'The data structure of ' + prop + ' is the same  as ' + link(p.alias) + '. See Details there.'
        if (p.cmd) asArray(p.cmd).forEach(_ => s += '\n' + pad + 'This option can also be used in its short-form in the comandline client `-' + _ + '` .')
        doc.push(s)
        if (typeof pt === 'object') {
            doc.push(pad + '    The ' + prop + ' object supports the following properties :\n' + pad)
            print_object(pt, pad + '    ', false, doc)
        }
        if (rpc_doc === doc) {
            if (p.example) doc.push('\n' + pad + '    *Example* : ' + prop + ': ' + JSON.stringify(p.example))
        }
        else if (config_doc === doc)
            asArray(p.example).forEach(ex => {
                key = prop
                doc.push(pad + '```sh')
                if (typeof (ex) == 'object')
                    doc.push(pad + '> ' + cmdName + ' ' + Object.keys(ex).filter(_ => typeof (ex[_]) !== 'object').map(k => '--' + pre + key + '.' + k + '=' + ex[k]).join(' ') + '  ....\n')
                else
                    doc.push(pad + [...asArray(p.cmd).map(_ => '-' + _), '--' + pre + key].map(_ => '> ' + cmdName + ' ' + _ + (ex === true ? '' : (_.startsWith('--') ? '=' : ' ') + ex) + '  ....').join('\n') + '\n')
                doc.push(pad + '```\n')
            })

        doc.push(pad + '\n')
    }
}


function handle_config(conf, pre, title, descr) {
    if (title) config_doc.push('\n## ' + title + '\n')
    for (const key of Object.keys(conf)) {
        const c = conf[key]
        if (typeof (c.type) === 'string' && types[c.type]) {
            c.typeName = c.type
            c.type = types[c.type]
        }
        // handle bindings
        generators.forEach(_ => _.updateConfig(pre, c, key, types))

        // handle doc
        if (!pre) {
            let s = '\n' + (title ? '#' : '') + '## ' + key + '\n\n' + c.descr
            if (c.optional) s += ' *This config is optional.*'
            if (c.default) s += ' (default: `' + JSON.stringify(c.default) + '`)'
            if (c.type) s += '\n\n Type: ' + typeName(c, true)
            if (c.enum) s += '\n\nPossible Values are:\n\n' + Object.keys(c.enum).map(v => '- `' + v + '` : ' + c.enum[v]).join('\n') + '\n'
            config_doc.push(s)
            if (typeof (c.type) === 'object') {
                config_doc.push('The ' + key + ' object supports the following properties :\n')
                print_object(c.type, '', false, config_doc, key + ".")
            }
            if (c.example !== undefined) {
                config_doc.push('\n*Example:*\n')
                asArray(c.example).forEach(ex => {
                    const s = '         '

                    config_doc.push('```eval_rst\n\n.. tabs::\n\n   .. code-tab:: sh\n')

                    if (typeof (ex) == 'object')
                        config_doc.push(s + '> ' + cmdName + ' ' + Object.keys(ex).filter(_ => typeof (ex[_]) !== 'object').map(k => '--' + pre + key + '.' + k + '=' + ex[k]).join(' ') + '  ....\n')
                    else
                        config_doc.push(s + [...asArray(c.cmd).map(_ => '-' + _), '--' + pre + key].map(_ => '> ' + cmdName + ' ' + _ + (ex === true ? '' : (_.startsWith('--') ? '=' : ' ') + ex) + '  ....').join('\n' + s) + '\n')
                    if (!title) {
                        config_doc.push('   .. code-tab:: js\n\n' + s + 'const ' + cmdName + ' = new ' + sdkName + '(' + JSON.stringify({ [key]: ex }, null, 2).split('\n').join('\n' + s) + ')\n\n')

                        config_doc.push('   .. code-tab:: c\n\n' + s + 'in3_configure(in3, "' + JSON.stringify({ [key]: ex }, null, 2).split('"').join('\\"').split('\n').join('\\\n' + s + '       ') + '");\n\n')

                        config_doc.push('   .. code-tab:: java\n\n' + s + sdkName + ' ' + cmdName + ' = new ' + sdkName + '(new ClientConfiguration(JSON.parse("""\n' + s + '           ' + JSON.stringify({ [key]: ex }, null, 2).split('\n').join('\n' + s + '           ') + '\n' + s + '           """)));\n\n')

                        config_doc.push('   .. code-tab:: swift\n\n' + s + 'let ' + ' ' + cmdName + ' = ' + sdkName + '( ' + JSON.stringify({ [key]: ex }, null, 2).split('\n').map(_ => {
                            let args = _.split(':')
                            if (args.length > 1) args[0] = args[0].split('"').join('').trim()
                            else return ''
                            return args.join(': ')
                        }).filter(_ => _).join('\n' + s + '           ') + ')\n\n')

                    }
                    config_doc.push('```\n')
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

function check_extension(api) {
    const aconf = api_conf[api]
    if (aconf.extension) {
        const fields = check_extension(aconf.extension.toLowerCase())
        if (fields) aconf.fields = { ...fields, ...aconf.fields }
    }
    return aconf.fields
}

async function main() {
    for (let s of src_dirs) await scan(s)
    Object.keys(api_conf).forEach(check_extension)
    docs.config.in3_config.params.config.type = config
    rpc_doc.push('# API RPC\n\n')
    rpc_doc.push('This section describes the behavior for each RPC-method supported with incubed.\n\nThe core of incubed is to execute rpc-requests which will be send to the incubed nodes and verified. This means the available RPC-Requests are defined by the clients itself.\n\n')
    config_doc.push('# Configuration\n\n')
    config_doc.push('When creating a new Incubed Instance you can configure it. The Configuration depends on the registered plugins. This page describes the available configuration parameters.\n\n')
    const sorted_rpcs = []
    for (const s of Object.keys(docs).sort()) {
        const rpcs = docs[s]
        const rdescr = api_conf[s].descr

        rpc_doc.push("## " + s + "\n\n")
        if (rdescr) rpc_doc.push(rdescr + '\n')

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
                    rpc_doc.push('*Returns:* ' + typeName(def.returns, true) + '\n\n' + def.returns.descr + '\n')
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

            let exampleList = asArray(def.example)
            if (exampleList.length) {
                for (const lang of Object.keys(examples)) {
                    let all = examples[lang][rpc]
                    if (all) {
                        while (all.length > exampleList.length) {
                            let l = exampleList.length
                            for (let i = 0; i < l; i++) exampleList.push({ ...exampleList[i] })
                        }
                        exampleList.forEach((ex, i) => {
                            ex.apis = ex.apis || {}
                            ex.apis[lang] = all[i % all.length]
                        })
                    }
                }
            }
            exampleList.forEach(ex => {
                const s = '         '
                const req = { method: rpc, params: ex.request || [] }
                if (def.proof) req.in3 = { "verification": "proof", ...ex.in3Params }
                const data = { result: ex.response || null }
                const is_json = (typeof data.result == 'object' || Array.isArray(data.result))
                if (ex.in3) data.in3 = ex.in3

                rpc_doc.push('*Example:*\n')
                if (ex.descr) rpc_doc.push('\n' + ex.descr + '\n')

                rpc_doc.push('```eval_rst\n\n.. tabs::\n\n')

                rpc_doc.push('\n   .. code-tab:: sh\n\n' + s + '> ' + cmdName + ' ' + (ex.cmdParams ? (ex.cmdParams + ' ') : '') + req.method + ' ' + (req.params.map(toCmdParam).join(' ').trim()) + (is_json ? ' | jq' : ''))
                rpc_doc.push(s + (is_json ? JSON.stringify(data.result, null, 2).split('\n').join('\n' + s) : '' + data.result))

                rpc_doc.push('\n   .. code-tab:: js jsonrpc\n\n' + s + '//---- Request -----\n\n' + s + JSON.stringify(req, null, 2).split('\n').join('\n' + s))
                rpc_doc.push('\n' + s + '//---- Response -----\n\n' + s + JSON.stringify(data, null, 2).split('\n').join('\n' + s))

                rpc_doc.push('\n   .. code-tab:: yaml\n\n' + s + '# ---- Request -----\n\n' + s + yaml.stringify(req).split('\n').join('\n' + s))
                rpc_doc.push('\n' + s + '//---- Response -----\n\n' + s + yaml.stringify(data).split('\n').join('\n' + s))

                for (const lang of Object.keys(ex.apis || {}).sort())
                    rpc_doc.push('\n   .. code-tab:: ' + lang + '\n\n' + s + ex.apis[lang].split('\n').join('\n' + s) + '\n')

                rpc_doc.push('```\n')

            })
            z += "'"
            zsh_cmds.push(z)
        }
        console.log('generate ' + s + '\n   ' + Object.keys(rpcs).join('\n   '))

        if (Object.values(rpcs).filter(_ => !_.skipApi).length)
            sorted_rpcs.push({
                api: s,
                conf: api_conf[s],
                rpcs,
                descr: rdescr,
                testCases: testCases[s]
            })
    }
    Object.keys(cmake_deps).forEach(m => { cmake_deps[m].depends = cmake_deps[m].depends.filter(_ => cmake_deps[_]) })
    generators.forEach(_ => _.generateAPI && sorted_rpcs.forEach(api => _.generateAPI(api.api, api.rpcs, api.descr, types, api.testCases, cmake)))
    generators.forEach(_ => _.generateAllAPIs && _.generateAllAPIs({ apis: sorted_rpcs, types, conf: cmake, cmake_deps, cmake_types }))

    handle_config(config, '')

    generators.forEach(_ => {
        _.generate_config()
        if (_.mergeExamples && examples && doc_dir.length && fs.existsSync(doc_dir[0]) && _.mergeExamples(examples))
            fs.writeFileSync(doc_dir[0] + '/rpc_examples.json', JSON.stringify(examples, null, 2), { encoding: 'utf8' })
    })

    handle_config(main_conf.config, '', 'cmdline options\n\nThose special options are used in the comandline client to pass additional options.\n')
    main_help.push('')
    main_help.push('In addition to the documented rpc-methods, those methods are also supported:')
    main_help.push('')
    Object.keys(main_conf.rpc).forEach(k => {
        (k + ' ' + main_conf.rpc[k]).split("\n").map(_ => _.trim()).map((_, i) => i ? '   ' + _ : _)
            .forEach(l => main_help.push(l))
    })

    if (zsh_file.length)
        fs.writeFileSync(zsh_file[0].replace('.template', '.sh'), fs.readFileSync(zsh_file[0], 'utf8').replace('$CMDS', zsh_cmds.join('\n')).replace('$CONFS', zsh_conf.join('\n')), { encoding: 'utf8' })
    if (doc_dir.length && fs.existsSync(doc_dir[0])) {
        fs.writeFileSync(doc_dir[0] + '/rpc.md', rpc_doc.join('\n') + '\n', { encoding: 'utf8' })
        fs.writeFileSync(doc_dir[0] + '/config.md', config_doc.join('\n') + '\n', { encoding: 'utf8' })
    }
    if (args_file.length)
        fs.writeFileSync(args_file[0], '// This is a generated file, please don\'t edit it manually!\n\n#include <stdlib.h>\n\nconst char* bool_props[] = {' + bool_props.map(_ => '"' + _ + '", ').join('') + 'NULL};\n\nconst char* help_args = "\\\n' + main_help.map(_ => _ + '\\n').join('\\\n') + '";\n\nconst char* aliases[] = {\n' + main_aliases.join('\n') + '\n    NULL};\n', { encoding: 'utf8' })

}
main().catch(console.error)