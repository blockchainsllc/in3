#!/usr/bin/env node

const yaml = require('yaml')
const fs = require('fs')
const { resolve, dirname } = require('path')
const { generate_openapi } = require('./openapi')
const { generate_solidity, create_abi_sigs } = require('./solidity')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    link,
    toCmdParam,
    typeName,
    create_example_arg,
    short_descr
} = require('./util')
const { create_rpc_doc, print_object } = require('./rpc_doc.js')

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
            let p = {}
            let type = '_api'
            cmake_data.apis.split(';').forEach(_ => {
                if (_.startsWith('_')) type = _
                else if (type == '_api') p = cmake.modules[_] = {}
                else p[type.substring(1)] = [...(p[type.substring(1)] || []), _]

            })
            cmake.options = {}
            fs.readFileSync(dirname(a.substr(8)) + '/CMakeCache.txt', 'utf8').split('\n').filter(_ => _.indexOf('=') >= 0 && !_.trim().startsWith('#')).forEach(op => {
                let [name, val] = op.split('=', 2)
                let [option, type] = name.split(':', 2)
                cmake.options[option] = type == 'BOOL' ? (val.toUpperCase() == 'ON' || val.toUpperCase() == 'TRUE') : val
            })
            const { dot, md } = create_modules()
            fs.writeFileSync(dirname(a.substr(8)) + '/mods.dot', dot, 'utf8')
            fs.writeFileSync(dirname(a.substr(8)) + '/mods.md', md, 'utf8')
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
    fs.readdirSync(p).filter(_ => _.startsWith('rpc_examples_')).forEach(l => {
        let lang = l.substring(13)
        lang = lang.substring(0, lang.indexOf('.json'))
        examples[lang] = { ...examples[lang], ...JSON.parse(fs.readFileSync(p + '/' + l, 'utf-8')) }
    })
})

//const doc_dir = process.argv[process.argv.length - 1]
const main_conf = yaml.parse(fs.readFileSync(in3_core_dir + '/c/src/cmd/in3/in3.yml', 'utf-8'))
const rpc_doc = []
const config_doc = []
const main_help = []
const main_aliases = []
const bool_props = []

let docs = {}, config = {}, types = {}, testCases = {}

function has_dep(a, b, stopper = []) {
    if (stopper.indexOf(a) >= 0) return false // circle detected
    stopper.push(a)
    let mod = cmake.modules[a]
    if (!mod || !mod.dep) return false
    if (mod.dep.indexOf(b) >= 0) return true
    return !!mod.dep.find(_ => has_dep(_, b, stopper))
}
function clean_deps(m) {
    let mod = cmake.modules[m]
    if (!mod || !mod.dep) return
    mod.dep.forEach(a => {
        mod.dep.filter(_ => _ != a).forEach(b => {
            if (has_dep(a, b)) mod.dep.splice(mod.dep.indexOf(b), 1)
        })
    })
}
function create_modules() {
    Object.keys(cmake.modules).forEach(clean_deps)
    Object.keys(cmake.modules).forEach(clean_deps)
    const groups = {}
    const exclude = ['nodeselect_def', 'transport_curl', 'multisig', "equs_config", "init", "usn_api"]
    const name = _ => _.substring(_.lastIndexOf(':') + 1)
    const getGroup = (name) => {
        if (name == 'core') name = 'in3'
        if (name == 'pay') name = 'verifier'
        if (!groups[name]) {
            groups[name] = { name, subs: [], head: [], apis: [], md: ['## ' + name + '\n'] }
            if (name != 'in3' && name != 'sdk') {
                getGroup('in3').subs.push(name)
                groups[name].parent = 'in3'
            }
        }
        return groups[name]
    }
    const createSub = g => {
        dot.push('subgraph cluster_' + g.name.replace('-', '_') + ' {')
        if (!g.parent) dot.push('      style=filled\n     fillcolor="#f2f2f2"\n    label=' + g.name.toUpperCase())
        else dot.push('      rank=min; label="' + g.name + '"')
        g.apis.forEach(_ => dot.push(_))
        g.subs.sort().forEach(_ => createSub(groups[_]))
        dot.push('     }')
    }
    let dot = [
        'digraph "blockchains_sdk" {',
        '    fontname="Helvetica,Arial,sans-serif"',
        '    newrank=true',
        '    nodesep=0.15',
        '    splines=ortho',
        '    # concentrate=true;',
        '    edge [arrowsize=0.5; penwidth=0.5]',
        '    node [ fontsize = "10"; fontcolor = "gray"; fontname="Arial"; color="gray"; margin=0; shape=component  ];',
    ]
    let used_services = {}

    Object.keys(cmake.modules || {}).sort().forEach(api => {
        if (exclude.indexOf(api) >= 0) return
        const mod = cmake.modules[api]
        const [dir] = mod.dir || ['']
        const in3_pos = (dir && dir.indexOf('/in3/c/src')) || -1
        const g = getGroup(in3_pos >= 0 ? dir.substring(in3_pos + 11).split('/')[0] : 'sdk')

        // get rpc_yml
        let apis = []
        try {
            let rpc = yaml.parse(fs.readFileSync(dir + '/rpc.yml', 'utf8'))
            apis = Object.keys(rpc).filter(_ => _ != 'types').map(name => ({
                name,
                def: rpc[name].api || {}
            }))
        } catch { }

        // collect data about services
        const services = apis.filter(_ => _.def.services).map(s => Object.keys(s.def.services).map(a => '\n    - ' + a + ' : ' + s.def.services[a]).join('')).join('')
        apis.forEach(api => Object.keys(api.def.services || {}).forEach(s => (used_services[s] || (used_services[s] = {}))[api.name] = api.def.services[s]))

        // create markdown
        g.md.push('### ' + api + '\n')
        if (mod.descr) g.md.push(mod.descr[0] + '\n')
        if (apis.length && apis[0].def.descr) g.md.push(apis[0].def.descr + '\n')
        g.md.push('- *location* : ' + (dir.substring(dir.lastIndexOf(g.name == 'sdk' ? '/src/' : '/in3/'))))
        g.md.push('- *depends on* : ' + (mod.dep ? mod.dep.map(_ => `\n    - [${_}](#${_.replace('_', '-')})`).join('') : ' no other module'))
        if (apis.length) g.md.push('- *APIs* : \n' + apis.map(_ => '    - ' + _.name).join('\n'))
        if (services.trim()) g.md.push('- *consumed Services* : ' + services)
        g.md.push('')

        g.apis.push(`   ${name(api)}  ${mod.register ? '[ style=filled; fillcolor="white"; shape ="box"; color="blue"; fontcolor="blue" ] ' : ''} `);
        (mod.dep || []).filter(_ => exclude.indexOf(_) == -1).forEach(_ => {
            if (!cmake.modules[_]) getGroup('third-party').apis.push('       ' + name(_))
        })
    })
    // create groups
    Object.values(groups).filter(_ => !_.parent).forEach(createSub)

    // create links
    Object.keys(cmake.modules || {}).sort().reverse().forEach(api => {
        if (exclude.indexOf(api) >= 0) return
        //        if (!cmake.modules[api].register) return
        (cmake.modules[api].dep || []).filter(_ => exclude.indexOf(_) == -1).forEach(_ =>
            dot.push(`   ${name(api)} -> ${name(_)} ${(cmake.modules[_] && (cmake.modules[_].register || []).length) ? '' : '[style=dashed; color=gray]'}`))
    })
    dot.push('}')



    let md = [
        '# Architecture\n',
        '```eval_rst\n',
        '.. graphviz::\n',
        dot.join('\n').split('\n').map(_ => '    ' + _).join('\n'),
        '\n```'
    ]
    if (groups.sdk) {
        md.push(groups.sdk.md.join('\n') + '\n')
        delete groups.sdk
    }

    Object.values(groups).forEach(_ => md.push(_.md.join('\n') + '\n'))

    md.push('\n## Consumed Services\n')
    Object.keys(used_services).forEach(s => {
        md.push('\n### ' + s + '\n')
        md.push("The following services are using the " + s + " backend:\n")
        Object.keys(used_services[s]).forEach(m => md.push('- [' + m + '](#' + m.replace('_', '-') + ') : ' + used_services[s][m]))
    })

    return { dot: dot.join('\n'), md: md.join('\n') }
}


function check_depends(n) {
    if (Array.isArray(n)) return check_depends(n[0])
    return (n && n.depends && cmake.modules)
        ? !asArray(n.depends).find(_ => !cmake.modules[_])
        : true
}

function add_testcase(k, t) {
    const first = asArray(t)[0]
    if (!first || (first.expected_output === undefined && first.expected_failure === undefined)) return false
    if (!check_depends(t)) return false

    if (testCases[k]) testCases[k] = [...asArray(testCases[k]), ...asArray(t)]
    else testCases[k] = asArray(t)
    return true
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
                if (!apic.api_dir || apic.descr) apic.api_dir = fullpath
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
                        await generate_solidity({ generate_rpc, types, api_name: k, api: ob[k], dir: fullpath, api_def: apic, allapis: docs })
                    Object.keys(ob[k]).forEach(_ => {
                        ob[k][_].src = fullpath
                        ob[k][_].generate_rpc = generate_rpc
                    })
                    cmake_types[fullpath] = true
                }
                /*
                if (!generators.length && apic.fields && lastAPI && docs[lastAPI]) {
                    for (const n of Object.keys(ob[k]).filter(_ => docs[lastAPI][_])) delete ob[k][n]
                    docs[lastAPI] = { ...docs[lastAPI], ...ob[k] }
                }
                else
                */
                docs[k] = { ...docs[k], ...ob[k] }
                lastAPI = k
            }
        }
        else if (f.name.startsWith("testCases") && f.name.endsWith('.yml') && is_valid) {
            console.error('parse ' + dir + '/' + f.name)
            const ob = yaml.parse(fs.readFileSync(dir + '/' + f.name, 'utf-8'))
            for (const k of Object.keys(ob)) {
                if (add_testcase(k, ob[k])) continue
                for (const t of Object.keys(ob[k])) add_testcase(t, ob[k][t])
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



function handle_config(ctx, conf, pre, title, descr) {
    if (title) config_doc.push('\n## ' + title + '\n')
    for (const key of Object.keys(conf)) {
        const c = conf[key]
        if (typeof (c.type) === 'string' && types[c.type]) {
            c.typeName = c.type
            c.type = types[c.type]
        }
        // handle bindings
        generators.forEach(_ => _.updateConfig(pre, c, key, types, ctx))

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
                print_object(ctx, c.type, '', false, config_doc, key + ".")
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
            handle_config(ctx, c.type, pre + key + '.')
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

function cleanup_depends() {
    let all = {}
    for (const api of Object.keys(docs)) all = { ...all, ...docs[api] }
    Object.keys(all).forEach(fn => {
        if (all[fn].depends && all[fn].depends.find(_ => !all[_])) all.skipApi = true
    })
}

function fix_examples() {
    // fix examples
    for (const api of Object.keys(docs)) {
        for (const fn of Object.keys(docs[api])) {
            const rpc = docs[api][fn]
            if (!rpc.example && !(rpc.returns || rpc.result || {}).options) {
                const tc = asArray(testCases[fn]).filter(_ => _.expected_output)[0]
                if (tc) {
                    rpc.example = {
                        request: tc.input,
                        response: tc.expected_output
                    }
                }
            }
        }
    }
}

async function main() {
    for (let s of src_dirs) await scan(s)

    // do we have extensions?
    Object.keys(api_conf).forEach(check_extension)

    // remove dependencies
    cleanup_depends()

    // cleanup depends
    fix_examples()




    docs.config.in3_config.params.config.type = config
    config_doc.push('# Configuration\n\n')
    config_doc.push('When creating a new Incubed Instance you can configure it. The Configuration depends on the registered plugins. This page describes the available configuration parameters.\n\n')
    let sorted_rpcs = []
    for (const s of Object.keys(docs).sort()) {
        const rpcs = docs[s]
        const rdescr = api_conf[s].descr

        for (const rpc of Object.keys(rpcs).sort()) {
            const def = rpcs[rpc]
            def.returns = def.returns || def.result
            def.result = def.returns || def.result
            let z = "    '" + rpc + ': ' + short_descr((def.descr || (def.alias && rpcs[def.alias].descr) || ''))
            if (def.params) z += ' ' + Object.keys(def.params).map(_ => '<' + _ + '>').join(' ')
            z += "'"
            zsh_cmds.push(z)
        }

        console.log('generate ' + s + '\n   ' + Object.keys(rpcs).map(k => k + (testCases[k] ? '' : ' '.padStart(70 - k.length) + ':: no testcase')).join('\n   '))

        sorted_rpcs.push({
            api: s,
            conf: api_conf[s],
            rpcs,
            descr: rdescr,
            testCases: Object.keys(rpcs).filter(k => !!testCases[k]).reduce((p, v) => { p[v] = testCases[v]; return p }, {})
        })
    }
    sorted_rpcs = sorted_rpcs.filter(a => Object.values(a.rpcs).filter(_ => !_.skipApi).length || sorted_rpcs.find(_ => camelCaseLow(a.api) == camelCaseLow(_.conf.extension || '')))
    const ctx = { config_doc, cmdName, examples, doc_dir, apis: sorted_rpcs, types, conf: cmake, cmake_deps, cmake_types, config, sdkName }
    Object.keys(cmake_deps).forEach(m => { cmake_deps[m].depends = cmake_deps[m].depends.filter(_ => cmake_deps[_]) })
    // fix examples
    sorted_rpcs.forEach(api => {
        Object.values(api.rpcs).filter(_ => _.result && !_.example).forEach(def =>
            def.example = {
                request: Object.keys(def.params || {}).filter(_ => !def.params[_].optional).map(k => create_example_arg(k, def.params[k], types)),
                response: create_example_arg('result', def.result || { type: 'string' }, types)
            }
        )
    })


    generators.forEach(_ => _.generateAPI && sorted_rpcs.forEach(api => _.generateAPI(api.api, api.rpcs, api.descr, types, api.testCases, cmake)))
    generators.forEach(_ => _.generateAllAPIs && _.generateAllAPIs(ctx))
    doc_dir.forEach(doc_dir => fs.existsSync(doc_dir) && create_rpc_doc(ctx))

    handle_config(ctx, config, '')

    generators.forEach(_ => {
        _.generate_config(ctx)
        if (_.mergeExamples && doc_dir.length && fs.existsSync(doc_dir[0])) {
            let exs = {}
            if (_.mergeExamples(exs))
                Object.keys(exs).forEach(lang => fs.writeFileSync(`${doc_dir[0]}/rpc_examples_${lang}.json`, JSON.stringify(exs[lang], null, 2), { encoding: 'utf8' }))
            examples = { ...examples, ...exs }
        }
        //        if (_.mergeExamples && examples && doc_dir.length && fs.existsSync(doc_dir[0]) && _.mergeExamples(examples))
        //            fs.writeFileSync(doc_dir[0] + '/rpc_examples.json', JSON.stringify(examples, null, 2), { encoding: 'utf8' })
    })

    handle_config(ctx, main_conf.config, '', 'cmdline options\n\nThose special options are used in the comandline client to pass additional options.\n')
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
        //        fs.writeFileSync(doc_dir[0] + '/rpc.md', rpc_doc.join('\n') + '\n', { encoding: 'utf8' })
        fs.writeFileSync(doc_dir[0] + '/config.md', config_doc.join('\n') + '\n', { encoding: 'utf8' })
    }
    if (args_file.length)
        fs.writeFileSync(args_file[0], '// This is a generated file, please don\'t edit it manually!\n\n#include <stdlib.h>\n\nconst char* bool_props[] = {' + bool_props.map(_ => '"' + _ + '", ').join('') + 'NULL};\n\nconst char* help_args = "\\\n' + main_help.map(_ => _ + '\\n').join('\\\n') + '";\n\nconst char* aliases[] = {\n' + main_aliases.join('\n') + '\n    NULL};\n', { encoding: 'utf8' })

    create_abi_sigs(in3_core_dir + '/c/src/api/eth1/abi_sigs.h')

}
main().then(_ => { console.log('done'); process.exit(0) }, console.error)