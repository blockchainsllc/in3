const fs = require('fs')
const jsonschema = require('jsonschema')
const yaml = require('yaml')
const schema = require('./rpc_schema.json')

const props = {
    properties: Object.keys(schema.$defs.property.properties).map(prop =>
        getProp(prop, schema.$defs.property.properties[prop], '')
    ).join(''),
    api: Object.keys(schema.$defs.api.properties).map(prop =>
        getProp(prop, schema.$defs.api.properties[prop], '')
    ).join(''),
    func: Object.keys(schema.$defs.func.properties).map(prop =>
        getProp(prop, schema.$defs.func.properties[prop], '')
    ).join(''),
    example: Object.keys(schema.$defs.example.properties).map(prop =>
        getProp(prop, schema.$defs.example.properties[prop], '')
    ).join('')
}

function getProp(prop, def, level) {
    let res = `${level}* **${prop}** : \`${getType(def)}\` - ${def.description}`
    let e = def.enum;
    (def.anyOf || []).forEach(_ => {
        if (_.enum) e = _.enum
    })
    if (e) res += '. Possible values are ' + e.map(_ => '`' + _ + '`').join(', ')
    let sub_ob = null
    if (def.type == 'object') sub_ob = def.properties
    else if (def.type == 'array' && def.items && def.items.type == 'object') sub_ob = def.items.properties
    if (sub_ob) {
        res += `\n${level}    The object${def.type == 'array' ? 's' : ''} may have the following properties:\n`
        res += Object.keys(sub_ob).map(p => getProp(p, sub_ob[p], level + '    ')).join('') + '\n'
    }
    return res + '\n'
}

function getType(def) {
    if (def.type) return def.type
    if (def.anyOf) {
        let types = []
        def.anyOf.forEach(_ => {
            let t = ''
            if (_.type == 'array' && _.items.type) t = _.items.type + '[]'
            else if (_.type) t = _.type
            else if (_.$ref) t = _.$ref.split('/').pop() + '-object'

            if (t && types.indexOf(t) < 0) types.push(t)
        })
        return types.join(' | ')
    }
    return 'object'
}

let template = fs.readFileSync(process.argv.pop(), 'utf8')
Object.keys(props).forEach(p => template = template.split('${' + p + '}').join(props[p]))
console.log(template)