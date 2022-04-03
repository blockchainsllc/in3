const fs = require('fs')
const yaml = require('yaml')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    addAll,
} = require('./util')
const compliance_header = [
    '/*******************************************************************************',
    ' * This file is part of the Incubed project.',
    ' * Sources: https://github.com/slockit/in3-c',
    ' *',
    ' * Copyright (C) 2018-2022 slock.it GmbH, Blockchains LLC',
    ' *',
    ' *',
    ' * COMMERCIAL LICENSE USAGE',
    ' *',
    ' * Licensees holding a valid commercial license may use this file in accordance',
    ' * with the commercial license agreement provided with the Software or, alternatively,',
    ' * in accordance with the terms contained in a written agreement between you and',
    ' * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further',
    ' * information please contact slock.it at in3@slock.it.',
    ' *',
    ' * Alternatively, this file may be used under the AGPL license as follows:',
    ' *',
    ' * AGPL LICENSE USAGE',
    ' *',
    ' * This program is free software: you can redistribute it and/or modify it under the',
    ' * terms of the GNU Affero General Public License as published by the Free Software',
    ' * Foundation, either version 3 of the License, or (at your option) any later version.',
    ' *',
    ' * This program is distributed in the hope that it will be useful, but WITHOUT ANY',
    ' * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A',
    ' * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.',
    ' * [Permissions of this strong copyleft license are conditioned on making available',
    ' * complete source code of licensed works and modifications, which include larger',
    ' * works using a licensed work, under the same license. Copyright and license notices',
    ' * must be preserved. Contributors provide an express grant of patent rights.]',
    ' * You should have received a copy of the GNU Affero General Public License along',
    ' * with this program. If not, see <https://www.gnu.org/licenses/>.',
    ' *******************************************************************************/'
]

const path = require('path');
function comment(ind, str) {
    return ind + '/**\n' + asArray(str).join('\n').split('\n').map(_ => ind + ' * ' + _ + '\n').join('') + ind + ' */'
}

exports.updateConfig = function (pre, c, key, types) { }

exports.generate_config = function () { }

exports.mergeExamples = function (all) { return all }

function getCType(def, name, index) {
    let c = ''
    switch (def.type) {
        case 'bool': return def.array
            ? {
                args: 'bool* ' + name + ', int ' + name + '_len',
                code_def: ''
            }
            : {
                args: 'bool ' + name,
                code_def: 'bool ' + name,
                code_read: def.optional
                    ? `TRY_PARAM_GET_BOOL(${name}, ctx, ${index}, ${def.default || 'false'})`
                    : `TRY_PARAM_GET_REQUIRED_BOOL(${name}, ctx, ${index})`,
                code_pass: name
            }
        case 'uint':
        case 'uint128':
        case 'int128':
        case 'uint256':
        case 'int256':
            return def.array
                ? {
                    args: 'bool* ' + name + ', int ' + name + '_len',
                    code_def: ''
                }
                : {
                    args: 'bytes_t ' + name,
                    code_def: 'bytes_t ' + name,
                    code_read: def.optional
                        ? `TRY_PARAM_GET_UINT256(${name}, ctx, ${index})`
                        : `TRY_PARAM_GET_REQUIRED_UINT256(${name}, ctx, ${index})`,
                    code_pass: name
                }
        case 'bytes':
            return def.array
                ? {
                    args: 'bool* ' + name + ', int ' + name + '_len',
                    code_def: ''
                }
                : {
                    args: 'bytes_t ' + name,
                    code_def: 'bytes_t ' + name,
                    code_read: def.optional
                        ? `TRY_PARAM_GET_BYTES(${name}, ctx, ${index}, ${def.minLength || 0}, ${def.maxLength || 0})`
                        : `TRY_PARAM_GET_REQUIRED_BYTES(${name}, ctx, ${index}, ${def.minLength || 0}, ${def.maxLength || 0})`,
                    code_pass: name
                }
        case 'string':
            return def.array
                ? {
                    args: 'bool* ' + name + ', int ' + name + '_len',
                    code_def: ''
                }
                : {
                    args: 'char* ' + name,
                    code_def: 'char* ' + name,
                    code_read: def.optional
                        ? `TRY_PARAM_GET_STRING(${name}, ctx, ${index}, ${def.default ? '"' + def.default + '"' : ''})`
                        : `TRY_PARAM_GET_REQUIRED_STRING(${name}, ctx, ${index})`,
                    code_pass: name
                }
        case 'int32':
        case 'uint32':
            return def.array
                ? {
                    args: 'bool* ' + name + ', int ' + name + '_len',
                    code_def: ''
                }
                : {
                    args: def.type + '_t ' + name,
                    code_def: 'int32_t ' + name,
                    code_read: def.optional
                        ? `TRY_PARAM_GET_INT(${name}, ctx,${index}, ${def.default || 0})`
                        : `TRY_PARAM_GET_REQUIRED_INT(${name}, ctx, ${index})`,
                    code_pass: (def.type == 'uint32' ? '(uint32_t)' : '') + ' ' + name
                }
        case 'uint64':
            return def.array
                ? {
                    args: 'bool* ' + name + ', int ' + name + '_len',
                    code_def: ''
                }
                : {
                    args: 'uint64_t ' + name,
                    code_def: 'uint64_t ' + name,
                    code_read: def.optional
                        ? `TRY_PARAM_GET_LONG(${name}, ctx, ${index}, ${def.default || 0})`
                        : `TRY_PARAM_GET_REQUIRED_LONG(${name}, ctx, ${index})`,
                    code_pass: name
                }
        default:
            return {
                args: 'd_token_t* ' + name,
                code_def: 'd_token_t* ' + name,
                code_read: def.optional
                    ? `TRY_PARAM_GET_OBJECT( ${name}, ctx, ${index})`
                    : `TRY_PARAM_GET_REQUIRED_OBJECT( ${name}, ctx, ${index})`,
                code_pass: name
            }

    }
}

function align_vars(items, ind) {
    let maxl = items.reduce((p, v) => Math.max(p, (v.trim().split(' ', 1)[0] || '').length), 0)
    return items.map(_ => _.trim()).map(_ => {
        let type = _.split(' ', 1)[0] || ''
        const rest = _.substring(type.length).trim()
        while (type.length < maxl) type += ' '
        return ind + type + ' ' + rest
    })
}

function generate_rpc(path, api_name, rpcs, descr, types) {
    const header = [
        compliance_header.join('\n') + '\n',
        comment('', "@file\n" + descr),
        `#ifndef ${api_name}_rpc_h__`,
        `#define ${api_name}_rpc_h__\n`,
        '#ifdef __cplusplus',
        'extern "C" {',
        '#endif\n',

        '#include "../../in3/c/src/core/client/client.h"',
        '#include "../../in3/c/src/core/client/plugin.h"',

        `#include "${api_name}.h"\n`,

        comment('', `handles the rpc commands for the ${api_name} modules.`),
        `in3_ret_t ${api_name}_rpc(${api_name}_config_t* conf, in3_rpc_handle_ctx_t* ctx);\n`
    ]

    const impl = [
        compliance_header.join('\n') + '\n',
        `#include "${api_name}_rpc.h"`,
        `#include "${api_name}.h"\n`,

        '#include "../../in3/c/src/core/client/keys.h"',
        '#include "../../in3/c/src/core/client/plugin.h"',
        '#include "../../in3/c/src/core/client/request_internal.h"',
        '#include "../../in3/c/src/core/util/debug.h"',
        '#include "../../in3/c/src/core/util/log.h"',
        '#include "../../in3/c/src/core/util/mem.h"',
        '#include "../../in3/c/src/core/util/utils.h"\n',
    ]

    const rpc_exec = []

    Object.keys(rpcs).filter(_ => _ != 'fields' && !_.startsWith('_')).forEach(rpc_name => {
        const r = rpcs[rpc_name];

        if (r.descr) {
            header.push(comment('', r.descr))
            impl.push(comment('', r.descr))
        }
        const params = []
        const code = {
            pre: [],
            read: [],
            pass: []
        }
        Object.keys(r.params || {}).forEach((p, i) => {
            const t = getCType(r.params[p], p, i)
            params.push(t.args)
            code.pre.push('  ' + t.code_def + ';')
            code.read.push('  ' + t.code_read)
            code.pass.push(t.code_pass)
        })
        header.push(`in3_ret_t ${rpc_name}(${api_name}_config_t* conf, in3_rpc_handle_ctx_t* ctx${params.length ? ', ' + params.join(', ') : ''});`)
        impl.push(`static in3_ret_t handle_${rpc_name}(${api_name}_config_t* conf, in3_rpc_handle_ctx_t* ctx) {`)
        align_vars(code.pre, '  ').forEach(_ => impl.push(_))
        code.read.forEach(_ => impl.push(_))
        impl.push(`  return ${rpc_name}(conf, ctx${params.length ? ', ' + code.pass.join(', ') : ''});`)
        impl.push('}\n')
        rpc_exec.push(`#if !defined(RPC_ONLY) || defined(RPC_${rpc_name.toUpperCase()})`)
        rpc_exec.push(`  TRY_RPC("${rpc_name}", handle_${rpc_name}(conf, ctx))`)
        rpc_exec.push('#endif\n')
    })

    header.push('\n#ifdef __cplusplus')
    header.push('}')
    header.push('#endif\n')
    header.push('#endif')

    fs.writeFileSync(path + `/${api_name}_rpc.h`, header.join('\n'), 'utf8')

    impl.push(comment('', 'handle rpc-requests and delegate execution'));
    impl.push(`in3_ret_t ${api_name}_rpc(${api_name}_config_t* conf, in3_rpc_handle_ctx_t* ctx) {`);
    impl.push(`  if (strncmp(ctx->method, "${api_name}_", ${api_name.length + 1})) return IN3_EIGNORE;\n`)
    rpc_exec.forEach(_ => impl.push(_))
    impl.push(`  return req_set_error(ctx->req, "unknown ${api_name} method", IN3_EUNKNOWN);`)
    impl.push('}')

    fs.writeFileSync(path + `/${api_name}_rpc.c`, impl.join('\n'), 'utf8')
}

exports.generateAPI = function (api_name, rpcs, descr, types, testCases) {
    let ext = ''
    const typeMapping = {}
    const imports = {}
    if (api_name === 'utils') api_name = 'util'

    if (rpcs._generate_rpc) generate_rpc(rpcs._generate_rpc, api_name, rpcs, descr, types)


    // checking for testcases
    if (testCases)

        // iterating through all testcases
        Object.keys(testCases).forEach(tc => createTestCaseFunction(tc, testCases[tc], api_name, rpcs[tc]))

}

function createTest(descr, method, tests, tc) {
    tests.push({
        descr,
        request: { method, params: tc.input || [] },
        result: getResult(tc.expected_output),
        config: tc.config || {},
        response: asArray(tc.mockedResponses).map(r => r.req.body?.params || r.res.result === undefined ? r.res : r.res.result)
    });
}
function getResult(x) {
    if (x && x.type && x.value !== undefined && Object.keys(x).length == 2) {
        if (typeof x.value === 'string') {
            if (x.type.startsWith("uint") && !x.value.startsWith('0x')) return parseInt(x.value)
        }
        return x.value
    }
    if (Array.isArray(x)) return x.map(getResult)
    return x
}

function createTestCaseFunction(testname, testCase, api_name, rpc) {

    let tests = [];
    if (!rpc) console.log("::: missing rpc-def for " + api_name + ' ' + testname)
    const rpcResult = rpc.result || {}
    asArray(testCase).forEach((t, index) => {
        const tn = t.descr || testname + (index ? ('_' + (index + 1)) : '')
        if (rpcResult.options && t.expected_output && t.expected_output.options)
            Object.keys(t.expected_output.options).forEach(k => {
                const tc = { ...t, input: [...t.input], expected_output: t.expected_output.options[k], mockedResponses: t.mockedResponses.options[k] }
                const option = rpcResult.options.find(_ => _.result && (_.result.type == k || _.name == k))
                if (option && option.params) {
                    Object.keys(option.params).forEach(prop => {
                        let i = Object.keys(rpc.params).indexOf(prop)
                        if (i < 0)
                            console.error("Invalid property " + prop + " in " + testname)
                        else {
                            while (tc.input.length <= 0) tc.input.add(0)
                            tc.input[i] = option.params[prop]
                        }
                    })
                    createTest(tn + '_' + k, testname, tests, tc)
                }
            })
        else
            createTest(tn, testname, tests, t)
    })

    const folders = [
        "../c/test/testdata/requests/generated",
        '../test/requests/generated'
    ]
    const folder = folders.find(f => fs.existsSync(f))

    let fullPath = folder + '/' + (testname.startsWith(api_name + '_') ? '' : (api_name + '_')) + testname + '.json'
    fs.writeFileSync(fullPath, JSON.stringify(tests, null, 2), 'utf8')
}
