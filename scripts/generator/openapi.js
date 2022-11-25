
const axios = require('axios')
const fs = require('fs')
const { dirname } = require('path')
const yaml = require('yaml')
const { snake_case, mergeTo, create_example_arg } = require('./util')



async function getDef(config) {
    const url = config.url
    if (!url) throw new Error('Missing url in openai generator config')
    let data = null
    if (url.startsWith('http'))
        data = await axios.default.get(url).then(_ => _.data)
    else
        data = fs.readFileSync(url, 'utf8')

    if (url.endsWith('.json'))
        return typeof (data) == 'string' ? JSON.parse(data) : data
    else if (url.endsWith('.yml') || url.endsWith('.yaml'))
        return yaml.parse(data)
    throw new Error('Unsupported url ' + url)
}

function get_fn_name(config, method, path, def) {
    const replaces = {
        post_didcomm: 'didcomm_send',
        post_feedback: 'handle_feedback',
        'post_proof-request': 'create_proof_request',
        'get_proof-request': 'get_proof_requests',
    }
    const action_prefixes = ['invite', 'changePrincipal', 'addRequest', 'retry', 'proof-request', 'receive', 'check', 'verify', 'login', 'register', 'reset', 'import', 'accept', 'logout', 'biometric', 'activate', 'deactivate', 'revoke', 'sign', 'send', 'transfer', 'cancel', 'prepare', 'submit', 'changePassword', 'withdraw', 'topup', 'create', 'validate', 'initiate', 'reset', 'kyc', 'verify', 'resend', 'start']
    let post_names = config.post_names || (config.post_names = {})
    let name = path
    let parts = name.split('/').filter(_ => _.trim())
    let args = parts.filter(_ => _.startsWith('{') || _.startsWith(':')).map(_ => _.substring(1).replace('}', ''))
    let i = parts.length - 1
    for (; i >= 0; i--) {
        if (!parts[i].startsWith('{') && !parts[i].startsWith(':')) break
    }
    let repl = replaces[`${method.toLowerCase()}_${parts[i]}`]
    if (repl) parts[i] = repl
    else if (action_prefixes.find(_ => parts[i].startsWith(_))) { }
    else if (method == 'get') {
        // If we have a required Id in path, we can assume that the endpoint requires identity and is therefore associated with a single domain model.
        const hasIdentity = def && def.parameters && def.parameters.find(param => param.in === "path" && param.name === "id" && !!param.required)
        const isAlreadyPlural = parts[i].endsWith('s') // This is a frail assumption (e.g.: access)
        const shouldAddTrailingS = !hasIdentity && !isAlreadyPlural
        parts[i] = 'get_' + parts[i] + (shouldAddTrailingS && i == parts.length - 1 ? 's' : '') + (args.length > 1 ? '_by_' + args[args.length - 1] : '')
    }
    else if (method == 'put')
        parts[i] = 'update_' + parts[i]
    else if (method == 'post')
        parts[i] = 'create_' + parts[i]
    else if (method == 'delete')
        parts[i] = 'delete_' + parts[i]

    if (parts[0] && ('' + parseInt(parts[0][0])) == parts[0][0]) parts[0] = 'exec_' + parts[0]

    name = config.api_name + '_' + (def.operationId || snake_case(parts.filter(_ => !_.startsWith('{') && !_.startsWith(':') && _.trim()).join('_')))

    if (post_names[name] && !def.operationId) {
        if (parts[parts.length - 1][0] == '{' || parts[parts.length - 1][0] == ':')
            name += '_by_' + args[args.length - 1]
        else
            throw new Error("duplicate name " + name)
    }

    const custom = (config.generate_rpc || {}).custom
    if (custom && custom[name]) {
        const alias = custom[name].alias
        if (alias) {
            custom[alias] = { ...custom[name] }
            delete custom[alias].alias
            name = alias
        }
    }

    post_names[name] = true
    return name
}

function resolve_ref(config, ref) {
    const [file, path] = ref.split('#', 2)
    let doc = config.data
    let res = path.split('/').filter(_ => _).reduce((val, p) => val[p], doc)
    if (res === undefined && file) res = path.split('/').filter(_ => _).reduce((val, p) => val[p], yaml.parse(fs.readFileSync(dirname(config.url) + '/' + file, 'utf8')))
    return res
}
function mergeSchemas(config, src) {
    let refs = src.oneOf || src.allOf
    if (!refs || !Array.isArray(refs)) return src
    let schema = {}
    refs.forEach(r => {
        let s = r.$ref ? { ...resolve_ref(config, r.$ref) } : { ...r }
        s = mergeSchemas(config, s)
        Object.keys(s).forEach(k => {
            switch (s) {
                case 'properties':
                    schema.properties = { ...schema.properties, ...s.properties }
                    break
                default:
                    if (Array.isArray(s[k])) schema[k] = [...(schema[k] || []), ...s[k]]
                    else schema[k] = s[k]
            }
        })
    })
    if (src.oneOf && schema.required) {
        // if this is an or, required are only those properties, which are required in all refs.
        const count = schema.required.reduce((p, c) => {
            p[c] = (p[c] || 0) + 1
            return p
        }, {})
        schema.required = Object.keys(count).filter(_ => count[_] == refs.length)
    }
    return schema
}

function get_type(config, content, names, parent = {}, example) {
    if (!content) return 'any'
    let _types = config._types || (config._types = {})
    let _types_names = config._types_names || (config._types_names = {})
    if (content['application/json']) content = content['application/json']
    let schema = content
    if (!example) example = schema.example
    if (content.schema) schema = content.schema
    if (schema.$ref) {
        names.splice(0, 0, snake_case(schema.$ref.split('/').pop()))
        schema = { ...resolve_ref(config, schema.$ref), ...schema }
    }
    if (!example && schema.example) example = schema.example
    if (example) parent.example = example
    if (schema.type == 'object' && !schema.properties && example && Array.isArray(example)) {
        // quickfix
        schema.type = 'array'
        schema.items = { type: example ? typeof (example[0]) : 'string' }
    }
    schema = mergeSchemas(config, schema)
    let type = schema.type || 'any'
    switch (type) {
        case 'boolean': return 'bool'
        case 'number': return 'uint64'
        case 'integer': return schema.format || 'uint64'
        case 'object': {
            let props = schema.properties
            let requiredProps = schema.requiredProperties || schema.required || []
            if (!props && example) {
                try {
                    if (typeof (example) == 'string') example = eval('_=' + example)
                }
                catch (ex) {
                    console.log(':::: The object ' + example + ' should be json!', ex)
                }
                props = {}
                // guess properties
                Object.keys(example).forEach(p => {
                    props[p] = {
                        type: Array.isArray(example[p]) ? 'array' : typeof (example[p]),
                        example: example[p],
                    }
                    if (props[p].type == 'array') {
                        const item = example[p][0]
                        props[p].items = { type: item ? typeof (item) : 'string' }
                    }
                })
                requiredProps = Object.keys(props)
            }
            if (!props || !Object.keys(props).length) return 'any'
            let prop_names = Object.keys(props)
            prop_names.sort()
            const ob_key = prop_names.join()
            const type_def = _types[ob_key] || (_types[ob_key] = { props, requiredProps })


            let name = type_def.name
            if (!name)
                for (let n of names) {
                    const nn = n.startsWith(config.api_name) ? n : config.api_name + '_' + n
                    if (!_types_names[nn] || _types_names[nn] === type_def) {
                        name = nn
                        _types_names[nn] = type_def
                        type_def.name = name
                        break
                    }
                }
            if (!name)
                for (let i = 1; i < 1000; i++) {
                    const nn = (names[0].startsWith(config.api_name) ? names[0] : config.api_name + '_' + names[0]) + i
                    if (!_types_names[nn] || _types_names[nn] === type_def) {
                        name = nn
                        _types_names[nn] = type_def
                        type_def.name = name
                        break
                    }
                }
            if (type_def.props !== props) {
                // merge props
                Object.keys(props).forEach(p => {
                    type_def.props[p] = { ...type_def.props[p], ...props[p] }
                })
                if (schema.requiredProperties) type_def.requiredProps = schema.requiredProperties
                if (schema.required) type_def.requiredProps = schema.required
            }

            // generate rpc-type
            type_def.type = {}
            Object.keys(type_def.props).forEach(p => {
                const def = type_def.props[p]
                const d = type_def.type[p] = {
                    descr: def.description || 'the ' + p
                }
                d.type = get_type(config, def, [p, name + '_' + p], d)
                if (type_def.requiredProps.indexOf(p) < 0) d.optional = true
                if (def.example) d.example = def.example
                if (def.enum) d.enum = def.enum
            })

            config.types[name] = type_def.type
            return name
        }

        case 'array': {
            parent.array = true
            return get_type(config, schema.items || {}, names, {}, example && example[0])
        }
        default: {
            if (schema.enum) {
                parent.enum = schema.enum
                type = 'string'
            }
            return type
        }
    }
}



function create_fn(config, method, path, def) {
    const base_name = snake_case(path.split('/').filter(_ => _.trim() && _[0] != '{' && _[0] != ':').join('_'))
    const fn_name = get_fn_name(config, method, path, def)
    const custom = ((config.generate_rpc || {}).custom || {})[fn_name]
    if (custom && custom.skipApi) return


    const fn = (config.api[fn_name] = {
        descr: def.description || def.summary,
        _generate_impl: impl_openapi
    })

    fn.descr += '\n\nThis function sends a `' + method.toUpperCase() + '` Request to **`' + path + '`**'
    fn._generate_openapi = { path, method }
    fn.params = {}
    path.split('/').filter(_ => _.startsWith('{') || _.startsWith(':')).map(_ => _.substring(1).replace('}', '')).forEach(p => {
        fn.params[snake_case(p)] = { descr: 'the ' + p, type: 'string' }
    })
    if (def.requestBody) {
        fn._generate_openapi.body = 'data'
        fn.params.data = {
            descr: 'the data object'
        }
        fn.params.data.type = get_type(config, def.requestBody.content || {}, [base_name, base_name + '_data', base_name + '_' + method + '_data'], fn.params.data)
        if (def.requestBody.require === false) fn.params.data.optional = true
    }
    if (def.parameters) {
        // This attempts to keep the params in the declared order while throwing anything that isnt required to the end (that is because some languages only accept optional params at the end)
        const requiredParams = def.parameters.filter(p => !!p.required)
        const optionalParams = def.parameters.filter(p => !p.required)
        const ordered = [...requiredParams, ...optionalParams]
        ordered.forEach(p => {
            const n = snake_case(p.name)
            if (p.in == 'query') {
                fn._generate_openapi.query = fn._generate_openapi.query || []
                fn._generate_openapi.query.push(p.name)
            } else if (p.in == 'body') {
                fn._generate_openapi.body = 'data'
                fn.params.data = {
                    descr: 'the data object'
                }
                fn.params.data.type = get_type(config, p, [n, base_name + '_' + (p.name || p)], fn.params.data)
                if (!(p.schema && p.schema.required && p.schema.required.length)) fn.params.data.optional = true
            }
            if (p.in != 'body') {
                let d = fn.params[n]
                if (!d) d = fn.params[n] = {}
                d.descr = p.description || 'the ' + n
                d.optional = !p.required
                d.type = get_type(config, p, [n, base_name + '_' + (p.name || p)], d)
            }
        })
    }

    const response = Object.keys(def.responses || {}).map(_ => {
        if (parseInt(_) < 400) {
            return def.responses[_]
        } else if (def.responses.RESPONSE) { // Not too sure what this is but its needed for CMS
            return def.responses.RESPONSE
        }
        return null
    }).find(_ => _)
    if (!response) throw new Error('no response found')
    fn.result = {
        descr: [response.summary, response.description].filter(_ => _).join('\n\n').trim()
    }
    if (response.content || (response.schema && response))
        fn.result.type = get_type(config, response.content || (response.schema && response), [base_name + '_result', base_name + '_' + method + '_result'], fn.result);
    else {
        fn._generate_openapi.empty_response = true
        fn.result = {
            ...fn.result,
            typeName: 'OperationResult',
            type: {
                success: {
                    type: 'bool',
                    descr: 'true if the function was successful. If not a error will be thrown.'
                }
            }
        }
    }

    fn.example = {
        request: Object.keys(fn.params).filter(_ => !fn.params[_].optional).map(k => create_example_arg(k, fn.params[k], config.types)),
        response: find_example(response, fn.result, config.types)
    }
    if (custom) mergeTo(custom, fn)
}
function find_example(el, def, types) {
    if (el.example) return el.example
    if (el.examples) {
        let first = Object.values(el.examples)[0]
        if (first && first.value) return first.value
        return first || el.examples
    }
    if (el.content) return find_example(el.content, def, types)
    if (el['application/json']) return find_example(el['application/json'], def, types)
    return create_example_arg('result', def, types)
}

function impl_add_param(res, qname, pdef, ind) {
    const name = snake_case(qname)

    if (pdef.array) {
        res.push(`${ind}for_children_of(iter, ${name})`)
        switch (pdef.type) {
            case 'string': res.push(`${ind}    sb_add_params(&_path, "${qname}=%s", d_string(iter.token));`); break
            case 'uint32': res.push(`${ind}    sb_add_params(&_path, "${qname}=%u", d_int(iter.token));`); break
            case 'uint64': res.push(`${ind}    sb_add_params(&_path, "${qname}=%U", d_long(iter.token));`); break
            default: throw new Error('invalid type in array ' + pdef.type + ' for ' + name)
        }
        return
    }
    switch (pdef.type) {
        case 'string': res.push(`${ind}sb_add_params(&_path, "${qname}=%s", ${name});`); break
        case 'uint32': res.push(`${ind}sb_add_params(&_path, "${qname}=%u", ${name});`); break
        case 'uint64': res.push(`${ind}sb_add_params(&_path, "${qname}=%U", ${name});`); break
        default: res.push(`${ind}sb_add_json(&_path, "", ${name});`); break

    }
}

function impl_openapi(fn, state) {
    const def = fn._generate_openapi
    const send = (state.generate_rpc || {}).send_macro || 'HTTP_SEND'
    const res = [`${send}("${def.method.toUpperCase()}",`]
    const parts = def.path.split('/')
    const args = []
    const ind = "".padStart(send.length + 1, ' ')

    for (let i = 0; i < parts.length; i++) {
        if (parts[i].startsWith('{') || parts[i].startsWith(':')) {
            const arg = snake_case(parts[i].substring(1).replace('}', ''))
            const pdef = fn.params[arg]
            if (!pdef) {
                throw new Error('missing parameter in path ' + arg)
            }
            args.push(arg)
            switch (pdef.type) {
                case 'string': parts[i] = '%s'; break
                case 'uint32': parts[i] = '%u'; break
                default: throw new Error('invalid type in path ' + pdef.type + ' for ' + arg)
            }
        }
    }
    if (args.length) res[res.length - 1] += ` sb_printx(&_path, "${parts.join('/')}", ${args.join(', ')});`
    else res[res.length - 1] += ` sb_add_chars(&_path, "${parts.join('/')}");`
    for (let q of def.query || []) {
        const pdef = fn.params[snake_case(q)]
        if (!pdef) throw new Error('missing query parameter in path ' + q)
        impl_add_param(res, q, pdef, ind)
    }
    if (def.body) switch (fn.params[def.body].type) {
        case 'string': res.push(`${ind}sb_add_chars(&_data, ${def.body});`); break
        case 'uint32': res.push(`${ind}sb_add_int(&_data, ${def.body});`); break
        case 'uint64': res.push(`${ind}sb_add_int(&_data, (int64_t) ${def.body});`); break
        default: res.push(`${ind}sb_add_json(&_data, "", ${state.generate_rpc && state.generate_rpc.structs ? def.body + '.json' : def.body});`);
    }

    res[res.length - 1] += ')'
    return res
}

function filter(t) {
    if (Array.isArray(t)) return t.map(filter)
    if (t === undefined || t === null) return null
    if (typeof t === 'object')
        return Object.keys(t).reduce((p, v) => {
            let value = t[v]
            if (typeof (value) === 'function') return p
            p[v] = filter(value)
            return p
        }, {})
    return t
}

const isHttpVerb = (m) => ['post', 'get', 'delete', 'put', 'patch', 'options', 'head'].includes(m)

exports.generate_openapi = async function (config) {
    config.data = await getDef(config)
    Object.keys(config.data.paths).forEach(_ =>
        Object.keys(config.data.paths[_])
            .filter(isHttpVerb) // we only filter verbs to ensure we are parsing actions or queries and not extra data
            .forEach(m => {
                create_fn(config, m, _, config.data.paths[_][m])
            })
    )

    //    fs.writeFileSync(config.api_name + '_types.yaml', yaml.stringify({ types: config.types || '' }))
    //    fs.writeFileSync(config.api_name + '_apis.yaml', yaml.stringify({ rpc: filter(config.api) }))

    //   config.api._generate_rpc = config.api._generate_rpc || {}
    //    config.api._generate_rpc.schema = config.data
    //    config.api._generate_rpc.schema = config.data

}
