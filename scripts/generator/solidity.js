const solc = require('solc')
const fs = require('fs')
const path = require('path')
const yaml = require('yaml')
const { snake_case, mergeTo, asArray } = require('./util')

function resolve_inputs(sources, dir) {
    while (true) {
        const missing = []
        Object.keys(sources).forEach(s => {
            const imports = sources[s].content.split('\n').filter(_ => _.startsWith('import ')).map(l => path.resolve(path.dirname(s) + '/' + l.split('"')[1]))
            imports.filter(_ => !sources[_] && missing.indexOf(_) < 0).forEach(_ => missing.push(_))
        })
        if (missing.length == 0) return;
        missing.forEach(_ => sources[_] = { content: fs.readFileSync(_, 'utf8') })
    }
}

function compile(ctx) {
    let input = {
        language: 'Solidity', sources: {}, settings: {
            "outputSelection": {
                "*": {
                    "*": [
                        "*"
                    ]
                }
            }
        }
    }
    ctx.files.forEach(f => input.sources[path.resolve(ctx.dir + '/' + f)] = { content: fs.readFileSync(ctx.dir + '/' + f, 'utf8') })
    resolve_inputs(input.sources, ctx.dir)
    const res = JSON.parse(solc.compile(JSON.stringify(input)))
    if (res.errors) throw new Error('Solidity Errors : ' + res.errors.map(_ => _.formattedMessage).join('\n'))

    let all_contracts = {}
    Object.keys(res.contracts).forEach(file => all_contracts = { ...all_contracts, ...res.contracts[file] })
    ctx.contract = all_contracts[ctx.sol.contract || Object.keys(all_contracts)[0]]

    if (!ctx.contract) throw new Error('Contract not found! : ' + Object.keys(all_contracts).join())
}

exports.generate_solidity = async function ({ generate_rpc, types, api_name, api, dir, api_def }) {
    let sol = generate_rpc.solidity
    if (!sol.prefix) sol.prefix = api_name
    const ctx = { files: asArray(sol.src), sol, types, api_name, api, generate_rpc, sol, dir }

    compile(ctx)
    create_def(ctx)

    api_def.descr = ctx.contract.devdoc.details || api_name
    api_def.fields = { contract: { descr: 'the address of the contract', type: 'address' }, ...api_def.fields }
}


function create_sig(fn, includeNames, includeOutput) {
    let sig = `${fn.name}(${fn.inputs.map(_ => (_.internalType || _.type) + (includeNames ? ' ' + _.name : '')).join()})`
    if (includeOutput) {
        sig += ':'
        if (fn.outputs.length == 1) sig += fn.outputs[0].type
        else sig += `(${fn.outputs.map(_ => (_.internalType || _.type) + (includeNames && _.name ? ' ' + _.name : '')).join()})`
    }
    return sig
}
function fix_type(t) {
    if (t.type && t.type.endsWith('[]')) {
        t.type = t.type.substring(0, t.type.length - 2)
        t.array = true
    }
    return t
}
function create_def(ctx) {
    const doc = ctx.contract.devdoc || {}
    const abi = ctx.contract.abi
    for (let fn of abi.filter(_ => _.type == 'function')) {
        let sig = create_sig(fn)
        let def = {
            descr: (doc.methods[sig] || {}).details || '',
            params: {
                contract: {
                    descr: 'the address of the contract',
                    type: 'address',
                    instance: 'contract',
                }
            },
            _generate_impl: impl_solidity,
            solidity: { ctx, fn }
        }
        ctx.api[ctx.sol.prefix + '_' + snake_case(fn.name)] = def
        fn.inputs.forEach(n => def.params[n.name] = fix_type({ type: n.internalType || n.type }))
        if (fn.stateMutability == 'view') {
            def.solidity.sig = create_sig(fn, true, true)
            def.result = {
                descr: "the resulting data structure"
            }
            if (fn.outputs.length == 1)
                def.result.type = fn.outputs[0].type
            else {
                def.result.type = {}
                fn.inputs.forEach((n, i) => def.result.type[n.name || 'p' + (i + 1)] = fix_type({ type: n.internalType || n.type, descr: n.name }))
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
        }



    }

}



function impl_solidity(fn, state, includes) {
    const abi_include = '#include "../../in3/c/src/api/eth1/abi.h"'
    const wallet_include = '#include "../wallet/wallet.h"'
    const l1_include = '#include "../l1_wallet/l1_wallet.h"'
    if (includes.indexOf(abi_include) < 0) includes.push(abi_include)
    if (includes.indexOf(wallet_include) < 0) includes.push(wallet_include)
    if (includes.indexOf(l1_include) < 0) includes.push(l1_include)
    const sol = fn.solidity
    let res = []
    if (sol.fn.stateMutability == 'view')
        res.push(`SEND_ETH_CALL${sol.fn.inputs.length == 0 ? '_NO_ARGS' : ''}(ctx, contract, "${sol.sig}"${sol.fn.inputs.map(_ => ', ' + _.name).join('')})`)
    else {
        res.push('TRY(wallet_check(ctx->req, &wallet, WT_ETH))')
        res.push('')
        res.push('tx_args_t arg    = {0};')
        res.push('arg.to           = contract;')
        res.push(`arg.data         = abi_encode_args(ctx, "${sol.sig}"${sol.fn.inputs.map(_ => ', ' + _.name).join('')});`)
        res.push('arg.gas          = 300000;')
        res.push('arg.wallet       = wallet;')
        res.push('arg.target_level = wallet_get_exec_level(exec, EXL_RECEIPT);')
        res.push('')
        res.push('if (ctx->req->error) return ctx->req->verification_state;')
        res.push('TRY_FINAL(l1_exec(ctx, &arg, NULL), _free(arg.data.data));')
        res.push('return IN3_OK;')
    }

    return res
}
