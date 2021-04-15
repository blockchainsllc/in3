#!/usr/bin/env node

const readline = require('readline');
var rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
});
let content = ''
let suite = ''
console.log("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n")
rl.on('line', line => {
    if (line.startsWith('Test Suite')) return

    if (line.startsWith('Test Case')) {
        if (line.indexOf('started') > 0) {
            const m = (/\[([a-zA-Z0-9\.]+)\s+(\w+)/gm).exec(line)
            content = ''
            if (m[1] != suite) {
                if (suite) console.log('  </testsuite>')
                console.log('  <testsuite name="' + (suite = m[1]) + '">')
            }
            console.log('    <testcase name="' + (suite = m[2]) + '">')
        }
        else {
            if (line.indexOf(' failed ') > 0)
                console.log('    <failure message="' + (content.trim()
                    .split('<').join('&lt;')
                    .split('>').join('&gt;')
                    .split('"').join('&quote;')
                ) + '"/>')
            console.log('    </testcase>')
        }
    }
    else
        content += line + '\n'
})
rl.on('close', () => {
    console.log("  </testsuite>\n</testsuites>")
})