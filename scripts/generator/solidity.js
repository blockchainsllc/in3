const fs = require('fs')
const path = require('path')
const yaml = require('yaml')
const { snake_case, mergeTo, asArray, camelCaseUp, create_example_arg } = require('./util')
const { exec } = require("child_process")
const all_hashes = {}
const solClasses = {}

function resolve_path(parent, import_path) {
    if (import_path.startsWith('@openzeppelin/')) {
        if (parent.indexOf('.cache') == -1) throw new Error('the module is not in the cache-folder')
        let parent_path = parent.split('/')
        let s = ''
        while (parent_path.pop() != '.cache') s += '../'
        let openzep = fs.readdirSync(parent_path.join('/') + '/.cache').find(_ => _.startsWith('openzeppelin'))
        if (!openzep) throw new Error('openzeppelin could not be found in the cache-folder')
        return s + openzep + import_path.replace('@openzeppelin', '')
    }
    if (import_path.startsWith('@gnosis.pm/safe-contracts/')) {
        if (parent.indexOf('.cache') == -1) throw new Error('the module is not in the cache-folder')
        let parent_path = parent.split('/')
        let s = ''
        while (parent_path.pop() != '.cache') s += '../'
        let safe = fs.readdirSync(parent_path.join('/') + '/.cache').find(_ => _.startsWith('safe-contracts'))
        if (!safe) throw new Error('safe could not be found in the cache-folder')
        return s + safe + import_path.substring(import_path.lastIndexOf('/contracts'))
    }
    return import_path
}
function resolve_inputs(sources, dir) {
    while (true) {
        const missing = []
        Object.keys(sources).forEach(s => {
            const imports = sources[s].content.split('\n').filter(_ => _.startsWith('import ') && _.indexOf('hardhat') < 0).map(l => {
                const src_path = l.split('"')[1]
                const dst_path = resolve_path(path.dirname(s), src_path)
                if (src_path != dst_path) sources[s].content = sources[s].content.replace(src_path, dst_path)
                return path.resolve(path.dirname(s) + '/' + dst_path)
            })
            imports.filter(_ => !sources[_] && missing.indexOf(_) < 0).forEach(_ => missing.push(_))
        })
        if (missing.length == 0) return;
        missing.forEach(_ => sources[_] = { content: fs.readFileSync(_, 'utf8') })
    }
}

function compile(ctx) {
    let input = {
        language: 'Solidity',
        sources: {},
        settings: {
            // See the solidity docs for advice about optimization and evmVersion
            optimizer: {
                enabled: true,
                runs: 200,
                details: {
                    yul: true,
                    yulDetails: {
                        stackAllocation: true,
                    },
                }
            },

            "outputSelection": {
                "*": {
                    "*": [
                        "*"
                    ]
                }
            }
        }
    }
    const cache = ctx.dir + '/.cache'
    const cacheFile = cache + '/' + ctx.api_name + '.json'
    if (!fs.existsSync(cache)) fs.mkdirSync(cache)
    const cachedTime = fs.existsSync(cacheFile) ? fs.lstatSync(cacheFile).mtime : null

    ctx.files.forEach(f => input.sources[path.resolve(ctx.dir + '/' + f)] = { content: fs.readFileSync(ctx.dir + '/' + f, 'utf8') })
    resolve_inputs(input.sources, ctx.dir)
    Object.keys(input.sources).forEach(file => {
        let lines = input.sources[file].content.split('\n').filter(line => {
            if (line.trim().startsWith('console.log')) return false
            if (line.indexOf('hardhat/console.sol') > 0) return false
            return true
        })
        lines = lines.map(_ => _.trim().startsWith('pragma solidity') ? 'pragma solidity ^0.8.13;' : _)
        if (!lines.find(_ => _.indexOf('// SPDX-License-Identifier:') >= 0)) lines.splice(0, 0, '// SPDX-License-Identifier: UNLICENSED')

        input.sources[file].content = lines.join('\n')
    })

    let res = undefined
    fs.writeFileSync(cache + '/' + ctx.api_name + '_input.json', JSON.stringify(input, null, 2), 'utf8')
    if (!cachedTime || Object.keys(input.sources).reduce((p, v) => Math.max(p, fs.lstatSync(v).mtime.getTime()), 0) > cachedTime.getTime()) {
        console.error(":: compiling ... ", ctx.files.join())
        const solc = require('solc')
        res = JSON.parse(solc.compile(JSON.stringify(input)))
        fs.writeFileSync(cacheFile, JSON.stringify(res, null, 2), 'utf8')
    }
    else
        res = fs.existsSync(cacheFile) ? JSON.parse(fs.readFileSync(cacheFile, 'utf8')) : null
    if (res.errors && res.errors.filter(_ => _.severity != "warning").length)
        throw new Error('Solidity Errors : ' + res.errors.filter(_ => _.severity == "error").map(_ => _.severity + ':::' + _.formattedMessage).join('\n'))

    let all_contracts = {}
    Object.keys(res.contracts).forEach(file => all_contracts = { ...all_contracts, ...res.contracts[file] })
    ctx.contract = all_contracts[ctx.sol.contract || Object.keys(all_contracts)[0]]

    if (!ctx.contract) throw new Error('Contract not found! : ' + Object.keys(all_contracts).join())
}


async function get_src(sol, dir) {
    if (sol.git) {
        let [url, version] = sol.git.split('#', 2)
        let repo_name = sol.git.split('/').pop().replace('.git', '').replace('#', '_')
        const cache = dir + '/.cache'
        const repo = cache + '/' + repo_name
        if (!fs.existsSync(repo)) {
            if (process.env.CI_JOB_TOKEN) url = url.replace('git@git.slock.it:', 'https://gitlab-ci-token:' + process.env.CI_JOB_TOKEN + '@git.slock.it/')
            console.error(":: cloning ... ", repo_name)
            await new Promise((resolve, reject) => {
                exec('git clone --single-branch --depth 1 ' + (version ? ('-b ' + version + ' ') : '') + url + ' ' + repo, (err, stdout, stderr) => {
                    if (err) reject(err)
                    else resolve(stdout)
                })
            })
        }
        return asArray(sol.src).map(_ => '.cache/' + repo_name + '/' + _)
    }
    else
        return asArray(sol.src)
}

exports.generate_solidity = async function ({ generate_rpc, types, api_name, api, dir, api_def, allapis }) {

    let sol = generate_rpc.solidity
    if (!sol.prefix) sol.prefix = api_name
    const ctx = { files: await get_src(sol, dir), sol, types, api_name, api, generate_rpc, sol, dir, api_def, allapis }

    compile(ctx)
    create_def(ctx)
    create_extends(ctx)

    api_def.descr = ctx.contract.devdoc.details || api_name
    api_def.fields = { contract: { descr: 'the address of the contract', type: 'address' }, ...api_def.fields }
}

function resolve_type(f, includeNames = true) {
    if (f.type == 'tuple')
        return '(' + f.components.map(_ => resolve_type(_, includeNames)).join() + ')'
    return f.type + (includeNames ? ' ' + f.name : '')
}
function create_sig(fn, includeNames, includeOutput) {
    let sig = `${fn.name || 'ctr'}(${fn.inputs.map(_ => resolve_type(_, includeNames)).join()})`
    if (includeOutput) {
        sig += ':'
        if (fn.outputs.length == 1) sig += fn.outputs[0].type
        else sig += `(${fn.outputs.map(_ => (_.type) + (includeNames && _.name ? ' ' + _.name : '')).join()})`
    }
    return sig
}
function fix_type(t, ctx, sig) {
    let type = t.type
    let res = { descr: t.descr || get_descr(ctx, sig, true, t.name) }
    if (type && type.indexOf('[') > 0) {
        type = type.split('[')[0]
        res.array = true
    }
    if (type == 'tuple') {
        type = {}
        if (t.internalType && t.internalType.startsWith('struct '))
            res.typeName = t.internalType.substring(7).replace('.', '')
        t.components.forEach(c => type[c.name] = fix_type(c))
    }
    res.type = type
    return res
}
function get_descr(ctx, sig, generate, param) {
    if (!sig && !param) return ''
    const descr = param
        ? d => ((d && d.params) ? (d.params[param] || d.params['_' + param]) : '') || ''
        : d => Object.keys(d || {}).filter(_ => _ != 'params').map(_ => d[_] + '\n\n').join('')
    const dev_doc = ctx && (ctx.contract.devdoc || {}).methods
    const user_doc = ctx && (ctx.contract.userdoc || {}).methods
    sig = sig ? sig.replace(/ \w+/g, '') : ''
    let d = (descr((user_doc || {})[sig]) + '\n\n' + descr((dev_doc || {})[sig])).trim()
    return d || (generate ? snake_case(param || sig.split('(', 1)[0] || '').split('_').join(' ') : '')
}
function create_def(ctx) {
    const abi = ctx.contract.abi
    const custom = (ctx.generate_rpc || {}).custom

    ctx.functions = []
    const deploy = ctx.sol.deploy && ctx.contract.evm.bytecode && ctx.contract.evm.bytecode.object && '0x' + ctx.contract.evm.bytecode.object
    if (deploy && !abi.find(_ => _.type == 'constructor')) abi.push({ type: 'constructor', inputs: [], "stateMutability": "nonpayable" })
    for (let fn of abi.filter(_ => _.type == 'function' || _.type == 'constructor')) {
        fn.inputs.forEach(_ => _.name = (_.name || 'ctr').replace('_', ''))
        let ctr = fn.type == 'constructor'
        let sig = create_sig(fn)
        let def = {
            descr: (ctr ? 'deploy the ' + camelCaseUp(ctx.abi_name) + ' contract.' : '') + '\n' + get_descr(ctx, sig, !ctr),
            params: {},
            _generate_impl: impl_solidity,
            solidity: { ctx, fn, deploy }
        }
        if (ctr && !deploy) continue
        if (!ctr) def.params.contract = {
            descr: 'the address of the contract',
            type: 'address',
            instance: 'contract',
        }

        ctx.functions.push(sig)
        const fn_name = ctr ? '' : (ctx.sol.prefix + '_' + snake_case(fn.name))
        if (ctr) {
            ctx.allapis[ctx.sol.deploy][ctx.sol.deploy + '_deploy_' + ctx.api_name] = def
            def.src = ctx.dir
            def.generate_rpc = ctx.generate_rpc
            def.cmakeOptions = ['MOD_CONTRACTS_DEPLOY']
        }
        else {
            const c = (custom && custom[fn_name]) || {}
            if (c.skipGenerate) continue
            ctx.api[fn_name] = def
            Object.assign(def, c)
        }


        fn.inputs.forEach(n => def.params[n.name] = fix_type(n, ctx, sig))
        if (fn.stateMutability == 'view') {
            def.solidity.sig = create_sig(fn, true, true)
            def._rpc_impl = `TRY_READ(FN_${fn_name.toUpperCase()}, "${def.solidity.sig}")`
            def.result = {
                descr: "the resulting data structure"
            }
            if (fn.outputs.length == 1)
                def.result = { ...def.result, ...fix_type(fn.outputs[0]) }
            else {
                def.result.type = {}
                fn.inputs.forEach((n, i) => def.result.type[n.name || 'p' + (i + 1)] = fix_type(n))
            }
        }
        else {
            def.solidity.sig = create_sig(fn, true, false)
            def.result = {
                descr: "The transaction data. Depending on the execl_level different properties will be defined.",
                type: 'tx_data',
            }
            def.params.exec = {
                descr: 'the execution level when sending transactions trough the wallet.\n' +
                    '    - `prepare` - the transaction is not signed, but for the multisig signatures all useable signatures are collected.\n' +
                    '    - `sign` - the raw transaction is signed\n' +
                    '    - `send` - the transaction is send and the transactionHash is added\n' +
                    '    - `receipt` - the function will wait until the receipt has been found',
                type: 'string',
                optional: true,
                default: 'send',
                example: 'prepare',
                enum: ['prepare', 'sign', 'send', 'receipt']
            }
            def.params.wallet = {
                descr: 'the wallet to be used. If ommited,  either the wallet as defined in the input-data is used, or the default-wallet as configured.',
                type: 'address',
                optional: true
            }
            let id = ctx.contract.evm.methodIdentifiers[create_sig(fn, false)]
            while (id && id.startsWith('0')) id = id.substring(1)
            if (id) all_hashes[def.solidity.sig] = id
            if (!ctr) def._rpc_impl = `TRY_SEND(FN_${fn_name.toUpperCase()}, "${def.solidity.sig}")`

        }
    }

}

function create_extends(ctx) {
    let found = null
    let match = 0
    Object.keys(solClasses).forEach(api => {
        const m = ctx.functions.reduce((p, c) => p + (solClasses[api].indexOf(c) == -1 ? 0 : 1), 0)
        if (m > match && m == solClasses[api].length) {
            found = api
            match = m
        }
    })
    solClasses[ctx.api_name] = ctx.functions
    if (found) {
        ctx.api_def.baseAPI = found
        ctx.functions
            .filter(s => solClasses[found].indexOf(s) >= 0) // filter the fncs existing in the baseclass
            .forEach(s => delete ctx.api[ctx.sol.prefix + '_' + snake_case(s.split('(', 1)[0])]) // and delete the definition in the API
    }

}


function chex(val) {
    let res = ''
    for (let i = 2; i < val.length; i += 2) {
        res += '\\x' + val.substring(i, i + 2)
    }
    return res
}
function impl_solidity(fn, state, includes) {
    const to_arg = _ => ', ' + ((_.components && !_.type.endsWith(']')) ? _.name + '.json' : _.name)
    const abi_include = '#include "../../in3/c/src/api/eth1/abi.h"'
    const wallet_include = '#include "../wallet/wallet.h"'
    const l1_include = '#include "../eth_wallet/eth_wallet.h"'
    const contracts_include = '#include "contracts.h"'
    if (includes.indexOf(abi_include) < 0) includes.push(abi_include)
    if (includes.indexOf(wallet_include) < 0) includes.push(wallet_include)
    if (includes.indexOf(l1_include) < 0) includes.push(l1_include)
    if (includes.indexOf(contracts_include) < 0) includes.push(contracts_include)
    const sol = fn.solidity
    let res = []
    if (sol.fn.stateMutability == 'view') {
        //   res.push(`SEND_ETH_CALL${sol.fn.inputs.length == 0 ? '_NO_ARGS' : ''}(ctx, contract, "${sol.sig}"${sol.fn.inputs.map(to_arg).join('')})`)
    }
    else if (sol.deploy && sol.fn.type == 'constructor') {
        let gas = sol.ctx.contract.evm.gasEstimates.creation.totalCost
        gas = gas == 'infinite' ? 460000 : parseInt(gas) + 500000
        res.push('TRY(wallet_check(ctx->req, &wallet, WT_ETH))')
        res.push('')
        res.push('tx_args_t arg    = {0};')
        res.push('arg.gas          = ' + gas + ';')
        res.push('arg.wallet       = wallet;')
        res.push('arg.target_level = wallet_get_exec_level(exec, EXL_RECEIPT);')
        res.push(`arg.data         = bytes((void*) "${chex(sol.deploy)}", ${sol.deploy.length / 2 - 1});`)
        if (sol.fn.inputs.length) {
            res.push(`bytes_t arg_data = abi_encode_args(ctx, "${sol.sig}"${sol.fn.inputs.map(to_arg).join('')});`)
            res.push('arg.data         = b_concat(2, arg.data, bytes(arg_data.data + 4, arg_data.len - 4));')
            res.push('_free(arg_data.data);')
            res.push('')
            res.push('if (ctx->req->error) return ctx->req->status;')
            res.push('TRY_FINAL(eth_exec(ctx, &arg, NULL), _free(arg.data.data));')
            res.push('return IN3_OK;')
        } else
            res.push('return eth_exec(ctx, &arg, NULL);')
    }
    /*
    else {
        res.push('TRY(wallet_check(ctx->req, &wallet, WT_ETH))')
        res.push('')
        res.push('tx_args_t arg    = {0};')
        res.push('arg.to           = bytes(contract, 20);')
        res.push(`arg.data         = abi_encode_args(ctx, "${sol.sig}"${sol.fn.inputs.map(to_arg).join('')});`)
        res.push('arg.gas          = 300000;')
        res.push('arg.wallet       = wallet;')
        res.push('arg.target_level = wallet_get_exec_level(exec, EXL_RECEIPT);')
        res.push('')
        res.push('if (ctx->req->error) return ctx->req->status;')
        res.push('TRY_FINAL(eth_exec(ctx, &arg, NULL), _free(arg.data.data));')
        res.push('return IN3_OK;')
    }
*/
    return res
}


exports.create_abi_sigs = function (path) {
    let content = [
        '#include <stdint.h>',
        'typedef struct {',
        '  uint32_t fn;',
        '  char*    signature;',
        '} abi_fn_t;\n',
        'const abi_fn_t abi_known_functions[] = {'
    ]
    Object.keys(all_hashes).forEach(k => content.push('    {.signature = "' + k + '", .fn = 0x' + all_hashes[k] + '},'))
    if (content[content.length - 1].endsWith(',')) content[content.length - 1] = content[content.length - 1].substring(0, content[content.length - 1].length - 1) + '};'
    else return
    fs.writeFileSync(path, content.join('\n'), 'utf8')
}