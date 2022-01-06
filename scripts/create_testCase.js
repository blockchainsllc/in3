#!/usr/bin/env node

const fs = require('fs')
const yaml = require('yaml')

function parse(content) {
    const commands = []
    content.split('\n').forEach(line => {
        if (line.startsWith(':: ')) {
            let args = line.substr(3).split(' ').map(_ => _.trim()).filter(_ => _)
            commands.push({
                action: args[0],
                args: args.slice(1),
                payload: ''
            })
        }
        else if (line.trim())
            commands[commands.length - 1].payload += line.trim()
    })
    return commands
}
function add_to_config(conf, key, value) {
    if (key == "fi") return
    const p = key.indexOf('=')
    if (p > 0) {
        key = key.substring(0, p)
        value = key.substring(p + 1)
    }
    if (value === undefined) throw new Error('Invalid config : ' + key)

    let t = key.split('.')
    const name = t.pop()
    t.forEach(o => { conf = conf[o] || (conf[o] = {}) })
    conf[name] = value
}

function resolve_alias(a) {
    const aliases = [
        "c", "chainId",
        "f", "finality",
        "a", "maxAttempts",
        "kin3", "keepIn3=true",
        "x", "experimental=true",
        "p", "proof",
        "l", "replaceLatestBlock",
        "s", "signatureCount",
        "bw", "bootWeights=true",
        "rc", "requestCount",
        "zks", "zksync.provider_url",
        "zkr", "zksync.rest_api",
        "zka", "zksync.account",
        "zsk", "zksync.sync_key",
        "zkat", "zksync.signer_type",
        "zms", "zksync.musig_pub_keys",
        "zmu", "zksync.musig_urls",
        "zc2", "zksync.create2",
        "zvpm", "zksync.verify_proof_method",
        "zcpm", "zksync.create_proof_method",
        "k", "key",
        "pk", "pk",
        "ccache", "clearCache=true",
        "e", "eth=true",
        "port", "port",
        "am", "allowed-methods",
        "b", "block",
        "to", "to",
        "from", "from",
        "d", "data",
        "gp", "gas_price",
        "gas", "gas",
        "token", "token",
        "nonce", "nonce",
        "test", "test",
        "path", "path",
        "st", "sigtype",
        "pwd", "password",
        "value", "value",
        "w", "wait=true",
        "json", "json=true",
        "hex", "hex=true",
        "debug", "debug=true",
        "q", "quiet=true",
        "h", "human=true",
        "tr", "test-request=true",
        "thr", "test-health-request=true",
        "ms", "multisig",
        "sigs", "ms.signatures",
        "ri", "response.in=true",
        "ro", "response.out=true",
        "fi", "fi",
        "fo", "file.out",
        "nl", "nodelist",
        "bn", "bootnodes",
        "os", "onlysign=true",
        "np", "proof=none",
        "ns", "stats=false",
        "v", "version=true",
        "fi", "fi",
        "h", "help=true"]

    for (let i = 0; i < aliases.length; i += 2) {
        if (aliases[i] == a)
            return aliases[i + 1]
    }

    throw new Error("Aliases for -" + a + ' not registered!')
}


function createTestCase(actions, out_file) {
    let fin = fs.existsSync(out_file) ? yaml.parse(fs.readFileSync(out_file, 'utf8')) : {}
    let config = {}
    let method = null
    let params = []
    let mockedResponses = []
    let last_request = null
    let expected_output = null
    let exec = null

    actions.forEach(action => {
        switch (action.action) {
            case 'cmd': {
                let k = null
                action.args.forEach(a => {
                    if (k) {
                        add_to_config(config, k, a)
                        k = null
                    }
                    else if (a.startsWith('--'))
                        add_to_config(config, a.substr(2))
                    else if (a.startsWith('-')) {
                        k = resolve_alias(a.substr(1))
                        if (k.indexOf('=') > 0) {
                            add_to_config(config, k)
                            k = null
                        }
                    }
                    else if (!exec) exec = a
                    else if (!method) method = a
                    else params.push(a)
                })
                break;
            }
            case 'request':
                last_request = JSON.parse(action.payload)
                break;

            case 'response':
                mockedResponses.push({
                    [last_request.method || last_request[0].method || 'in3_http']: { req: last_request, res: JSON.parse(action.payload) }
                })
                break;
            case 'result':
                expected_output = JSON.parse(action.payload)
                break;

        }
    })

    // find a method....
    if (Object.keys(fin).length == 0) fin[method.indexOf('_') > 0 ? method.substr(0, method.indexOf('_')) : 'testCases'] = {}
    let api = fin[Object.keys(fin).find(k => fin[k][method]) || Object.keys(fin).pop()]
    let testCase = { config, input: params, expected_output, mockedResponses }
    if (!api[method]) api[method] = testCase
    else if (Array.isArray(api[method])) api[method].push(testCase)
    else api[method] = [api[method], testCase]

    fs.writeFileSync(out_file, yaml.stringify(fin), 'utf8')
}


const out_file = process.argv.pop()
createTestCase(parse(fs.readFileSync(process.argv.pop(), 'utf8')), out_file)


