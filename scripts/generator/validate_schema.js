const fs = require('fs')
const jsonschema = require('jsonschema')
const yaml = require('yaml')
const schema = require('./rpc_schema.json')
const files = process.argv.filter(_ => _.endsWith('.yml'))
let types = {}
let apis = {}
let content = {}
files.forEach(file => {
    const data = yaml.parse(fs.readFileSync(file, 'utf8'))
    content[file] = data
    if (file.endsWith("rpc.yml")) {
        Object.keys(data.types || {}).forEach(_ => types[_] = data.types[_])
        Object.keys(data).filter(_ => _ != 'types').forEach(_ => apis[_] = { ...apis[_], ...data[_] })
        const result = jsonschema.validate(data, schema)
        if (result.errors.length) {
            for (let err of result.errors)
                console.error('Error in ' + file + ' : ' + err.toString().replace('instance.', ''))
            process.exitCode = 1
        }
    }
})

files.forEach(file => {
    const data = content[file]
    if (file.endsWith("rpc.yml")) {
        Object.keys(data.types || {}).forEach(type => {
            check_properties("Error in " + file + ': The predefined type ' + type, data.types[type])
        })
        Object.keys(data).filter(_ => !_.startsWith('types')).forEach(api => {
            const api_data = data[api]
            Object.keys(api_data).filter(_ => !_.startsWith('_') && _ != 'fields' && _ != 'descr' && _ != 'config').forEach(func => {
                check_properties("Error in " + file + ': The function ' + func + ' in params', api_data[func].params || {})
                check_type("Error in " + file + ': The function ' + func + ' in result', (api_data[func].result || {}).type)
            })

        })

    } else if (file.split('/').pop().startsWith("test")) {
        Object.keys(data).forEach(api => {
            //            console.error(`### check api in ${file} : ${api} `)

            if (apis[api]) {
                Object.keys(data[api]).forEach(test => {
                    if (!apis[api][test]) {
                        console.error(`Error in ${file} : The test is not associated with a rpc-function : ${test} `)
                        process.exitCode = 1
                    } else {
                        apis[api][test].has_test = true

                    }
                    // TODO check test
                })
            }
            //            else 
            //                console.error(`::: unknown api in ${file} : ${api} `)
        })
    }
})

Object.keys(apis).forEach(api => {
    Object.keys(apis[api]).filter(_ => _ != 'api' && !apis[api][_].has_test).forEach(func => {
        console.error(`Warning : The ${func}  has no testdata `)
    })

})

function check_properties(prefix, props) {
    Object.keys(props).filter(_ => !_.startsWith('_')).forEach(prop => {
        check_type(prefix + ` with property ${prop} : `, props[prop].type)
    })

}

function check_type(prefix, t) {
    if (!t) return
    if (typeof (t) === 'object') {
        return check_properties(prefix + '/type', t)
    }
    switch (t) {
        case 'uint32':
        case 'uint64':
        case 'uint16':
        case 'bytes':
        case 'bytes32':
        case 'bytes96':
        case 'address':
        case 'bool':
        case 'uint256':
        case 'int':
        case 'byte':
        case 'bytes256':
        case 'bytes128':
        case 'bytes4':
        case 'any':
        case 'string':
        case 'double':
            return
        default:
            if (types[t]) return
    }
    console.error(prefix + ` Type '${t}' is unknown!`)
    process.exitCode = 1
}