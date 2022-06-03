const axios = require('axios')
const fs = require('fs')
const { dirname } = require('path')
const yaml = require('yaml')
const { snake_case, mergeTo } = require('./util')
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
    const action_prefixes = ['check', 'verify', 'login', 'register', 'reset', 'import', 'accept', 'logout', 'biometric', 'activate', 'deactivate']
    let post_names = config.post_names || (config.post_names = {})
    let name = path
    let parts = name.split('/').filter(_ => _.trim())
    let args = parts.filter(_ => _.startsWith('{')).map(_ => _.substring(1, _.length - 1))
    let i = parts.length - 1
    for (; i >= 0; i--) {
        if (!parts[i].startsWith('{')) break
    }
    if (action_prefixes.find(_ => parts[i].startsWith(_))) { }
    else if (method == 'get')
        parts[i] = 'get_' + parts[i] + (i == parts.length - 1 ? 's' : '') + (args.length > 1 ? '_by_' + args[args.length - 1] : '')
    else if (method == 'put')
        parts[i] = 'update_' + parts[i]
    else if (method == 'post')
        parts[i] = 'create_' + parts[i]
    else if (method == 'delete')
        parts[i] = 'delete_' + parts[i]

    if (parts[0] && ('' + parseInt(parts[0][0])) == parts[0][0]) parts[0] = 'exec_' + parts[0]

    name = config.api_name + '_' + parts.filter(_ => !_.startsWith('{') && _.trim()).join('_')

    if (post_names[name]) {
        if (parts[parts.length - 1][0] == '{')
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
    return snake_case(name)
}

function resolve_ref(config, ref) {
    const [file, path] = ref.split('#', 2)
    let doc = config.data
    if (file) doc = yaml.parse(fs.readFileSync(dirname(config.url) + '/' + file, 'utf8'))

    return path.split('/').filter(_ => _).reduce((val, p) => val[p], doc)
}

function get_type(config, content, names, parent = {}) {
    if (!content) return 'string'
    let _types = config._types || (config._types = {})
    let _types_names = config._types_names || (config._types_names = {})
    if (content['application/json']) content = content['application/json']
    let schema = content
    if (schema.example) parent.example = schema.example
    if (content.schema) schema = content.schema
    if (schema.$ref) {
        names.splice(0, 0, snake_case(schema.$ref.split('/').pop()))
        schema = { ...resolve_ref(config, schema.$ref), ...schema }
    }
    if (schema.example) parent.example = schema.example
    if (schema.type == 'object' && !schema.properties && schema.example && Array.isArray(schema.example)) {
        // quickfix
        schema.type = 'array'
        schema.items = { type: 'string' }
    }
    let type = schema.type || 'any'
    switch (type) {
        case 'boolean': return 'bool'
        case 'number': return 'uint32'
        case 'integer': return schema.format || 'uint32'
        case 'object': {
            let props = schema.properties
            let requiredProps = schema.requiredProperties || schema.required || []
            if (!props && content.example) {
                props = {}
                // guess properties
                Object.keys(content.example).forEach(p => {
                    props[p] = {
                        type: Array.isArray(content.example[p]) ? 'array' : typeof (content.example[p]),
                        example: content.example[p],
                    }
                    if (props[p].type == 'array') {
                        const item = content.example[p][0]
                        props[p].items = { type: typeof (item) || 'string' }
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
            return get_type(config, schema.items || {}, names, {})
        }
        default: {
            if (schema.enum) parent.enum = schema.enum
            return type
        }
    }
}

function create_fn(config, method, path, def) {
    const base_name = snake_case(path.split('/').filter(_ => _.trim() && _[0] != '{').join('_'))
    const fn_name = get_fn_name(config, method, path, def)
    const custom = ((config.generate_rpc || {}).custom || {})[fn_name]
    if (custom && custom.skipApi) return


    const fn = (config.api[fn_name] = {
        descr: def.description || def.summary,
        _generate_impl: impl_openapi
    })

    fn._generate_openapi = { path, method }
    fn.params = {}
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
            }
            const d = {
                descr: p.description || 'the ' + p
            }
            fn.params[n] = d
            if (!p.required) d.optional = true // default is not required
            d.type = get_type(config, p, [n, base_name + '_' + (p.name || p)], d)
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
        descr: response.description || ''
    }
    fn.result.type = get_type(config, response.content, [base_name + '_result', base_name + '_' + method + '_result'], fn.result);
    if (custom) mergeTo(custom, fn)
}

function impl_add_param(res, qname, pdef, ind) {
    const name = snake_case(qname)

    if (pdef.array) {
        res.push(`${ind}for (d_iterator_t iter = d_iter(${name}); iter.left; d_iter_next(&iter))`)
        switch (pdef.type) {
            case 'string': res.push(`${ind}    sb_add_params(&_path, "${qname}=%s", d_string(iter.token));`); break
            case 'uint32': res.push(`${ind}    sb_add_params(&_path, "${qname}=%u", d_int(iter.token));`); break
            default: throw new Error('invalid type in array ' + pdef.type + ' for ' + name)
        }
        return
    }
    switch (pdef.type) {
        case 'string': res.push(`${ind}sb_add_params(&_path, "${qname}=%s", ${name});`); break
        case 'uint32': res.push(`${ind}sb_add_params(&_path, "${qname}=%u", ${name});`); break
        default: res.push(`${ind}sb_add_json(&_path, "", ${qname});`); break

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
        if (parts[i].startsWith('{')) {
            const arg = snake_case(parts[i].substring(1, parts[i].length - 1))
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
        default: res.push(`${ind}sb_add_json(&_data, "", ${def.body});`);
    }

    res[res.length - 1] += ')'
    return res
}

exports.generate_openapi = async function (config) {
    config.data = await getDef(config)
    Object.keys(config.data.paths).forEach(_ =>
        Object.keys(config.data.paths[_]).forEach(m => create_fn(config, m, _, config.data.paths[_][m]))
    )
    //    config.api._generate_rpc = config.api._generate_rpc || {}
    //    config.api._generate_rpc.schema = config.data
    //    config.api._generate_rpc.schema = config.data

}
