const fs = require("fs")
const crypto = require('crypto')
const path = require("path")
var stdinBuffer = fs.readFileSync(0)
const input = stdinBuffer.toString().split('\n').filter(_ => !(_.startsWith('cc1:') || _.startsWith('make[') || _.startsWith('make:')))
const root = path.resolve(process.argv.pop())

//input.forEach(_ => console.log('::: ' + _))
const res = []
input.forEach(line => {
    const regex = /^(\/.*?):([0-9]+):([0-9]+):\s*([a-z]+):\s*(.*)/m;
    let m;
    if ((m = regex.exec(line)) !== null) {
        const [all, path, lin, col, level, description] = m
        res.push({
            description,
            fingerprint: '',
            severity: level == 'warning' ? 'major' : 'critical', // info, minor, major, critical, or blocker
            location: {
                path: path.substring(root.length + 1),
                lines: {
                    begin: parseInt(lin)
                }
            }
        })
    }
    else if (res.length) res[res.length - 1].description += '\n' + line
})
console.log(JSON.stringify(res, null, 2))
if (res.length) process.exitCode = 1
