const fs = require("fs")
const crypto = require('crypto')
const path = require("path")
const { url } = require("inspector")
var stdinBuffer = fs.readFileSync(0)
const input = stdinBuffer.toString().split('\n').filter(_ => !(_.startsWith('cc1:') || _.startsWith('make[') || _.startsWith('make:')))
const tool = process.argv.pop()
const root = path.resolve(process.argv.pop())

//input.forEach(_ => console.log('::: ' + _))
const res = []
input.forEach((line, i) => {
    const valgrind = /==([0-9]+)==\sERROR SUMMARY:\s([0-9]+)\ errors/g;
    const regex = /^(\/.*?):([0-9]+):([0-9]+):\s*([a-z]+):\s*(.*)/m;
    let m;
    if ((m = valgrind.exec(line)) !== null && m[2] != '0') {
        let content = ''
        let last_path = ''
        let last_line = 0
        let last_method = ''
        let is_malloc = false
        //        ==2129==    by 0x4243F6: eth_set_pk_signer (c/src/signer/pk-signer/signer.c:215)

        for (let n = i; n >= 0; n--) {
            const r = /==([0-9]+)==.*?:\s+([0-9a-zA-Z_]+)\s+\(([0-9a-zA-Z_/\.\-]+):([0-9]+)\)/g;
            if (!input[n].startsWith('==' + m[1] + '==')) break;
            let mm = r.exec(input[n])
            if (mm && !is_malloc) {
                last_line = parseInt(mm[4])
                last_path = mm[3]
                last_method = mm[2]
            }
            if (!is_malloc && input[n].indexOf('malloc') >= 0) is_malloc = true
            content = input[n].substring(4 + m[1].length) + '\n' + content
        }
        const id = crypto.createHash('sha256')
            .update(content, 'utf8')
            .digest('hex')
        res.push({
            id,
            category: 'sast',
            message: 'Valgrind Memory Leak in ' + last_method,
            cve: '',
            scanner: {
                id: tool,
                name: tool
            },
            description: '',
            raw_source_code_extract: '',
            severity: 'Critical',
            location: {
                file: last_path,
                start_line: last_line
            },
            details: {
                path: {
                    type: "code",
                    lang: 'c',
                    value: content
                }
            },
            confidence: 'High',
            identifiers: [{
                "type": tool,
                "name": tool + " warning",
                "value": tool
            }]
        })
    }
    else if ((m = regex.exec(line)) !== null) {
        const [all, path, lin, col, level, message] = m
        if (level == 'note' || path.indexOf('third-party') > 0) return
        const id = crypto.createHash('sha256')
            .update(path.substring(root.length + 1), 'utf8')
            .update(message, 'utf8')
            .digest('hex')
        res.push({
            id,
            category: 'sast',
            message,
            cve: '',
            scanner: {
                id: tool,
                name: tool
            },
            description: '',
            raw_source_code_extract: '',
            severity: level == 'note' ? 'Info' : (level == 'warning' ? 'Medium' : 'Critical'), // info, minor, major, critical, or blocker
            location: {
                file: path.substring(root.length + 1),
                start_line: parseInt(lin)
            },
            details: {
                path: {
                    type: "code",
                    lang: 'c',
                    value: ''
                }
            },
            confidence: 'High',
            identifiers: [{
                "type": tool,
                "name": tool + " warning",
                "value": tool
            }]
        })

        const cwe = /\[CWE-([0-9]+)\]/g
        if ((m = regex.exec(message)) !== null) {
            res[res.length - 1].details.more = {
                type: 'url',
                href: 'https://cwe.mitre.org/data/definitions/' + m[1] + '.html'
            }
            res[res.length - 1].identifiers.push({
                "type": "cwe",
                "name": "CWE-" + m[1],
                "value": m[1],
                "url": "https://cwe.mitre.org/data/definitions/" + m[1] + ".html"
            })

        }
    }
    else if (res.length && tool != 'valgrind') {
        res[res.length - 1].raw_source_code_extract += '\n' + line
        res[res.length - 1].details.path.value += '\n' + line
    }
})
//res.forEach(_ => _.description = '```\n' + _.description + '\n```\n')
console.log(JSON.stringify({ version: '14.0.4', vulnerabilities: res }, null, 2))
if (res.length) process.exitCode = 1
