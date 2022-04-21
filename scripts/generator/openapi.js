const axios = require('axios')
const fs = require('fs')
const yaml = require('yaml')
const { snake_case } = require('./util')
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

    name = config.api_name + '_' + parts.filter(_ => !_.startsWith('{') && _.trim()).join('_')

    if (post_names[name]) {
        if (parts[parts.length - 1][0] == '{')
            name += '_by_' + args[args.length - 1]
        else
            throw new Error("duplicate name " + name)
    }
    post_names[name] = true
    return snake_case(name)
}

function get_type(config, content, names, parent = {}) {
    if (!content) return 'string'
    let _types = config._types || (config._types = {})
    let _types_names = config._types_names || (config._types_names = {})
    if (content['application/json']) content = content['application/json']
    let schema = content
    if (schema.example) parent.example = schema.example
    if (content.schema) schema = content.schema
    if (schema.example) parent.example = schema.example
    let type = schema.type || 'string'
    switch (type) {
        case 'boolean': return 'bool'
        case 'number': return 'uint32'
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
                        props[p].items = { type: typeof (item) }
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
            //            console.log('add type ' + name, type_def.type)
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
    const fn = (config.api[fn_name] = {
        descr: def.description || def.summary
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
    if (def.parameters)
        def.parameters.forEach(p => {
            const n = snake_case(p.name)
            if (p.in == 'query') {
                fn._generate_openapi.query = fn._generate_openapi.query || []
                fn._generate_openapi.query.push(p.name)
            }
            const d = {
                descr: p.description || 'the ' + p
            }
            fn.params[n] = d
            if (p.required === false) d.optional = true
            d.type = get_type(config, p, [n, base_name + '_' + p], d)
        })
    const response = Object.keys(def.responses || {}).map(_ => parseInt(_) < 400 ? def.responses[_] : null).find(_ => _)
    if (!response) throw new Error('no response found')
    fn.result = {
        descr: response.description || ''
    }
    fn.result.type = get_type(config, response.content, [base_name + '_result', base_name + '_' + method + '_result'], fn.result)



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



if (require.main === module) {
    const types = {}
    const api = {}
    //    const url = 'https://equs.git-pages.slock.it/interop/ssi/ssi-core/swagger-build.yaml' // process.argv.pop()
    //const url = '/Users/simon/ws/crypto/kms/api-spec/public/swagger.json'
    const url = '/Users/simon/ws/sdk/sdk-core/src/id/openapi.yml'

    exports.generate_openapi({ url, api_name: 'id', api, types }).then(() => {
        console.log(yaml.stringify({ types, id: api }))
    }, console.error)

}
