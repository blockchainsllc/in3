const fs = require('fs')
const jsonschema = require('jsonschema')
const yaml = require('yaml')
const schema = require('./rpc_schema.json')

process.argv.filter(_ => _.endsWith('rpc.yml')).forEach(file => {
    const data = yaml.parse(fs.readFileSync(file, 'utf8'))
    const result = jsonschema.validate(data, schema)
    if (!result.errors.length)
        console.log(`${file} is valid.`)
    else {
        for (let err of result.errors)
            console.log('Error in ' + file + ' : ' + err.toString().replace('instance.', ''))
        process.exitCode = 1
    }
})

