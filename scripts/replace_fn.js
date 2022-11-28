const fs = require('fs')
const _path = require('path')
const fn_names = {}
const filenames = []

function scan(dir) {
    console.log(dir)
    fs.readdirSync(dir, { withFileTypes: true }).forEach(f => {
        const p = dir + '/' + f.name
        if (f.isDirectory()) scan(p)
        else if (f.name == 'rpcs.h') fs.readFileSync(p, 'utf8').split('\n').filter(_ => _.startsWith('#define FN_')).map(_ => _.substring(8).trim().split(' ')).forEach(([c, name]) => fn_names[name] = { name: c, path: p })
        else if (f.name.endsWith('.c')) filenames.push(p)
    })
}

function replace(path) {
    console.log('check ' + path)
    let content = fs.readFileSync(path, 'utf8')
    let modified = false
    Object.keys(fn_names).forEach(f => {
        let s = content.split(f)
        if (s.length > 1) {
            modified = true
            let rpcs = '#include "' + _path.relative(_path.dirname(path), fn_names[f].path) + '"'
            if (s[0].indexOf(rpcs) == -1) {
                const p = s[0].indexOf('#include ');
                s[0] = s[0].substring(0, p) + rpcs + '\n' + s[0].substring(p)
            }
            content = s.join(fn_names[f].name)
        }
    })
    if (modified) fs.writeFileSync(path, content)
}

scan(process.argv.pop())
filenames.forEach(replace)
