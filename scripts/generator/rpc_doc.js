const yaml = require('yaml')
const fs = require('fs')
const { resolve, dirname } = require('path')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    link,
    typeName,
    toCmdParam,
    short_descr,
    create_example_arg
} = require('./util')

const title = (val, head) => `${head || ''} ${val}\n\n`


function print_object(ctx, def, pad, useNum, doc, pre, handled = []) {

    let i = 1
    for (const prop of Object.keys(def)) {
        let s = pad + (useNum ? ((i++) + '.') : '*') + ' **' + prop + '**'
        const p = def[prop]
        while (typeof p.type == 'string' && typeof (ctx.types[p.type]) === 'string') p.type = ctx.types[p.type]

        const pt = getType(p.type, ctx.types)
        const tname = p.type && typeName(p, true)
        if (p.type) s += ' : ' + tname
        if (p.optional) s += ' *(optional)*'
        if (p.descr) s += ' - ' + p.descr
        if (p.key) s += ' with ' + p.key + ' as keys in the object'
        if (p.default) s += ' (default: `' + JSON.stringify(p.default) + '`)'
        if (p.enum) s += '\n' + pad + 'Possible Values are:\n\n' + Object.keys(p.enum).map(v => pad + '    - `' + v + '` : ' + p.enum[v]).join('\n') + '\n'
        if (p.alias) s += '\n' + pad + 'The data structure of ' + prop + ' is the same  as ' + link(p.alias) + '. See Details there.'
        if (p.cmd) asArray(p.cmd).forEach(_ => s += '\n' + pad + 'This option can also be used in its short-form in the comandline client `-' + _ + '` .')
        doc.push(s)
        if (typeof pt === 'object' && !(tname && handled.indexOf(tname) >= 0)) {
            doc.push(pad + '    The ' + prop + ' object supports the following properties :\n' + pad)
            print_object(ctx, pt, pad + '    ', false, doc, undefined, [...handled, tname])
        }
        if (ctx.rpc_doc === doc) {
            if (p.example) doc.push('\n' + pad + '    *Example* : ' + prop + ': ' + JSON.stringify(p.example))
        }
        else if (ctx.config_doc === doc)
            asArray(p.example).forEach(ex => {
                key = prop
                doc.push(pad + '```sh')
                if (typeof (ex) == 'object')
                    doc.push(pad + '> ' + ctx.cmdName + ' ' + Object.keys(ex).filter(_ => typeof (ex[_]) !== 'object').map(k => '--' + pre + key + '.' + k + '=' + ex[k]).join(' ') + '  ....\n')
                else
                    doc.push(pad + [...asArray(p.cmd).map(_ => '-' + _), '--' + pre + key].map(_ => '> ' + ctx.cmdName + ' ' + _ + (ex === true ? '' : (_.startsWith('--') ? '=' : ' ') + ex) + '  ....').join('\n') + '\n')
                doc.push(pad + '```\n')
            })

        doc.push(pad + '\n')
    }
}
function create_api(ctx, { api, conf, rpcs, descr }, head = '', h = '') {
    if (!h) h = api
    const rpc_doc = ctx.rpc_doc
    let prefix = ''
    if (Object.keys(rpcs).length) {
        rpc_doc.push(title(h, head + '#'))
        if (descr) rpc_doc.push(descr + '\n')
    }
    else
        prefix = h + '.'


    // sub apis
    ctx.apis.filter(_ => _.conf.extension && camelCaseUp(_.conf.extension) == camelCaseUp(api) && _.conf.extensionVar)
        .forEach(_ => create_api(ctx, _, prefix ? head : head + '#', prefix + _.conf.extensionVar))

    // rpcs
    for (const rpc of Object.keys(rpcs).sort()) {
        const def = rpcs[rpc]

        rpc_doc.push(title(rpc, head + '##'))
        asArray(def.alias).forEach(_ => rpc_doc.push(rpc + ' is just an alias for ' + link(_) + '.See Details there.\n\n'))
        if (def.descr)
            rpc_doc.push(def.descr + '\n')
        if (def.params) {
            rpc_doc.push("*Parameters:*\n")
            print_object(ctx, def.params, '', true, rpc_doc)
            rpc_doc.push()
        }
        else if (!def.alias)
            rpc_doc.push("*Parameters:* - \n")
        if (def.in3Params) {
            rpc_doc.push('The following in3-configuration will have an impact on the result:\n\n');
            print_object(ctx, getType(def.in3Params, ctx.types), '', false, rpc_doc)
            rpc_doc.push()
        }
        if (def.validation) rpc_doc.push('\n' + def.validation + '\n')

        if (def.returns) {
            if (def.returns.type) {
                rpc_doc.push('*Returns:* ' + typeName(def.returns, true) + '\n\n' + def.returns.descr + '\n')
                const pt = getType(def.returns.type, ctx.types)
                if (typeof pt === 'object') {
                    rpc_doc.push('\nThe return value contains the following properties :\n')
                    print_object(ctx, pt, '', false, rpc_doc)
                }
            }
            else if (def.returns.alias)
                rpc_doc.push('*Returns:*\n\nThe Result of `' + rpc + '` is the same as ' + link(def.returns.alias) + '. See Details there.\n')
            else
                rpc_doc.push('*Returns:*\n\n' + (def.returns.descr || '') + '\n')
        }

        if (def.proof) {
            rpc_doc.push('*Proof:*\n\n' + (def.proof.descr || '') + '\n')
            const pt = getType(def.proof.type, ctx.types)
            if (def.proof.alias)
                rpc_doc.push('The proof will be calculated as described in ' + link(def.proof.alias) + '. See Details there.\n\n')

            if (pt) {
                rpc_doc.push("This proof section contains the following properties:\n\n")
                print_object(ctx, pt, '', false, rpc_doc)
                rpc_doc.push("\n\n")
            }
        }

        if (!def.example)
            def.example = {
                request: Object.keys(def.params || {}).filter(_ => !def.params[_].optional).map(k => create_example_arg(k, def.params[k], ctx.types)),
                response: create_example_arg('result', def.result || { type: 'string' }, ctx.types)
            }




        let exampleList = asArray(def.example)
        if (exampleList.length) {
            for (const lang of Object.keys(ctx.examples)) {
                let all = ctx.examples[lang][rpc]
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

            rpc_doc.push('\n   .. code-tab:: sh\n\n' + s + '> ' + ctx.cmdName + ' ' + (ex.cmdParams ? (ex.cmdParams + ' ') : '') + req.method + ' ' + (req.params.map(toCmdParam).join(' ').trim()) + (is_json ? ' | jq' : ''))
            rpc_doc.push(s + (is_json ? JSON.stringify(data.result, null, 2).split('\n').join('\n' + s) : '' + data.result))

            rpc_doc.push('\n   .. code-tab:: js jsonrpc\n\n' + s + '//---- Request -----\n\n' + s + JSON.stringify(req, null, 2).split('\n').join('\n' + s))
            rpc_doc.push('\n' + s + '//---- Response -----\n\n' + s + JSON.stringify(data, null, 2).split('\n').join('\n' + s))

            rpc_doc.push('\n   .. code-tab:: yaml\n\n' + s + '# ---- Request -----\n\n' + s + yaml.stringify(req).split('\n').join('\n' + s))
            rpc_doc.push('\n' + s + '//---- Response -----\n\n' + s + yaml.stringify(data).split('\n').join('\n' + s))

            for (const lang of Object.keys(ex.apis || {}).sort())
                rpc_doc.push('\n   .. code-tab:: ' + lang + '\n\n' + s + ex.apis[lang].split('\n').join('\n' + s) + '\n')

            rpc_doc.push('```\n')

        })
    }
}


exports.create_rpc_doc = function (ctx) {
    //    ctx.rpc_doc = []
    //    ctx.rpc_doc.push('# API RPC\n\n')
    //    ctx.rpc_doc.push('This section describes the behavior for each RPC-method supported with incubed.\n\nThe core of incubed is to execute rpc-requests which will be send to the incubed nodes and verified. This means the available RPC-Requests are defined by the clients itself.\n\n')
    ctx.apis.filter(_ => (!_.conf.sdk_getter || typeof (_.conf.sdk_getter) === 'string') && !_.conf.extension && !_.conf.fields).forEach(_ => {
        ctx.rpc_doc = []
        create_api(ctx, _, '', _.api)
        fs.writeFileSync(ctx.doc_dir + '/rpc_' + _.api + '.md', ctx.rpc_doc.join('\n') + '\n', { encoding: 'utf8' })
    })
}
exports.print_object = print_object