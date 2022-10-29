const fs = require('fs')
const { relative } = require('path')
const yaml = require('yaml')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    addAll,
    snake_case,
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
    ' *******************************************************************************/',
    '',
    '// ::: This is a autogenerated file. Do not edit it manually! :::'
]

const path = require('path');
function comment(ind, str) {
    return ind + '/**\n' + asArray(str).join('\n').trim().split('\n').map(_ => ind + ' * ' + _ + '\n').join('') + ind + ' */'
}

exports.updateConfig = function (pre, c, key, types) { }

exports.generate_config = function () { }

exports.mergeExamples = function (all) { return all }


function resolveTypeDependency(typename, state) {
    if (typeof (typename) != 'string') return null
    const def_dir = state.cmake_types[typename]
    if (!def_dir || def_dir == state.path || !state.cmake_types[def_dir]) return null
    // find relative_path
    const p = def_dir.split('/')
    return path.relative(state.path, def_dir + '/' + p[p.length - 1] + '_rpc.h')
}

function getBool(def, name, index) {
    return {
        args: 'bool ' + name,
        code_def: 'bool ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_BOOL(${name}, ctx, ${index}, ${def.default || 'false'})`
            : `TRY_PARAM_GET_REQUIRED_BOOL(${name}, ctx, ${index})`,
        code_pass: name
    }
}
function getUint(def, name, index) {
    const len = parseInt(def.type.substring(def.type.startsWith('uint') ? 4 : 3) || "256")
    if (len < 33)
        return {
            args: def.type + '_t ' + name,
            code_def: 'int32_t ' + name,
            code_read: def.optional
                ? `TRY_PARAM_GET_INT(${name}, ctx, ${index}, ${def.default || 0})`
                : `TRY_PARAM_GET_REQUIRED_INT(${name}, ctx, ${index})`,
            code_pass: (def.type == 'uint32' ? '(uint32_t)' : '') + ' ' + name,
            validate: validate(def, name, 'ctx->req', name)
        }
    else if (len < 65)
        return {
            args: 'uint64_t ' + name,
            code_def: 'uint64_t ' + name,
            code_read: def.optional
                ? `TRY_PARAM_GET_LONG(${name}, ctx, ${index}, ${def.default || 0})`
                : `TRY_PARAM_GET_REQUIRED_LONG(${name}, ctx, ${index})`,
            code_pass: name,
            validate: validate(def, name, 'ctx->req', name)
        }
    else
        return {
            args: 'bytes_t ' + name,
            code_def: 'bytes_t ' + name,
            code_read: def.optional
                ? `TRY_PARAM_GET_UINT256(${name}, ctx, ${index})`
                : `TRY_PARAM_GET_REQUIRED_UINT256(${name}, ctx, ${index})`,
            code_pass: name,
            validate: validate(def, name, 'ctx->req', name)
        }
}
function getBytes(def, name, index) {
    const len = parseInt(def.type.substring(5) || "0")
    const enc = def.encoding_param_index !== undefined ? `, d_encoding_from_string(d_get_string_at(ctx->params, ${def.encoding_param_index}), ENC_HEX)` : ''
    return {
        args: 'bytes_t ' + name,
        code_def: 'bytes_t ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_BYTES${enc ? '_AS' : ''}(${name}, ctx, ${index}, ${def.minLength || len}, ${def.maxLength || len}${enc})`
            : `TRY_PARAM_GET_REQUIRED_BYTES${enc ? '_AS' : ''}(${name}, ctx, ${index}, ${def.minLength || len}, ${def.maxLength || len}${enc})`,
        code_pass: name,
        validate: validate(def, name, 'ctx->req', name)
    }
}
function getAddress(def, name, index) {
    return {
        args: 'uint8_t* ' + name,
        code_def: 'uint8_t* ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_ADDRESS(${name}, ctx, ${index}, NULL)`
            : `TRY_PARAM_GET_REQUIRED_ADDRESS(${name}, ctx, ${index})`,
        code_pass: name,
        validate: validate(def, name, 'ctx->req', name)
    }
}

function get_key_hash(name) {
    let val = 0
    for (let l = 0; l < name.length; l++)
        val ^= (name.charCodeAt(l) | val << 7) & 0xFFFF
    return '0x' + val.toString(16)
}

function getString(def, name, index) {
    return {
        args: 'char* ' + name,
        code_def: 'char* ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_STRING(${name}, ctx, ${index}, ${def.default ? '"' + def.default + '"' : 'NULL'})`
            : `TRY_PARAM_GET_REQUIRED_STRING(${name}, ctx, ${index})`,
        code_pass: name,
        validate: validate(def, name, 'ctx->req', name)
    }
}
function get_type_name(name) {
    let r = name[0].toLowerCase()
    for (let i = 1; i < name.length; i++) {
        if (name[i] >= 'A' && name[i] <= 'Z' && !(name[i - 1] >= 'A' && name[i - 1] <= 'Z') && name[i - 1] != '_') r += '_'
        r += name[i].toLowerCase()
    }
    switch (r) {
        case 'unsigned': return 'unsigned_raw';
        case 'signed': return 'signed_raw';
        default: return r
    }
}

function defineType(type_name, type, types, api, type_defs, descr, init, src_type) {
    let required = ''
    let simple_typename = type_name.substring(4, type_name.length - 2)
    let struct_vars = [
        '  d_token_t* json; // json-token'
    ]
    const def = {
        header: '/** ' + descr + ' */\ntypedef struct {\n',
        impl: `in3_ret_t ${convert_fn_name(api, type_name)}(in3_req_t* r, d_token_t* ob, ${type_name}* val) {\n` +
            `  if (d_type(ob) != T_OBJECT) return rpc_throw(r, "Invalid %s object", "${simple_typename}");\n` +
            '  val->json = ob;\n' +
            '  for_children_of(iter, ob) {\n' +
            '    switch (d_get_key(iter.token)) {\n'
    }
    // TODO handle default-values
    Object.keys(type).forEach(prop => {
        def.impl += `      case ${get_key_hash(prop)}: // ${prop}\n`
        const pt = type[prop]
        const prop_name = get_type_name(prop)
        const descr = (pt.descr || prop).split('\n').join(' ')
        const opt_check = pt.optional ? 'd_type(iter.token) != T_NULL && ' : ''
        let is_ob = false
        let req = ''
        if (pt.array) {
            struct_vars.push(`  d_token_t* ${prop_name}; // ${descr}`)
            def.impl += `        if (d_type(iter.token) != T_ARRAY) return rpc_throw(r, "%s.%s must be an array.", "${prop}", "${simple_typename}");\n`
            def.impl += `        val->${prop_name} = iter.token;\n`
            req = `!val->${prop_name}`
        }
        else if (typeof (pt.type) === 'string') {
            if (pt.type.startsWith("uint") || pt.type.startsWith('int')) {
                const signed = pt.type.startsWith('int')
                const len = parseInt(pt.type.substring(signed ? 3 : 4) || "256")
                if (len < 33) {
                    struct_vars.push(`  ${signed ? 'int32_t' : 'uint32_t'} ${prop_name}; // ${descr}`)
                    def.impl += `        if (!d_is_bytes(iter.token) && d_type(iter.token) != T_INTEGER && !d_num_bytes(iter.token).data) return rpc_throw(r, "%s.%s must be a integer.", "${prop}", "${simple_typename}");\n`
                    if (signed)
                        def.impl += `        val->${prop_name} = d_int(iter.token);\n`
                    else
                        def.impl += `        val->${prop_name} = (uint32_t) d_long(iter.token);\n`
                }
                else if (len < 65) {
                    struct_vars.push(`  ${signed ? 'int64_t' : 'uint64_t'} ${prop_name}; // ${descr}`)
                    def.impl += `        if (!d_is_bytes(iter.token) && d_type(iter.token) != T_INTEGER && !d_num_bytes(iter.token).data) return rpc_throw(r, "%s.%s must be a integer.", "${prop}", "${simple_typename}");\n`
                    def.impl += `        val->${prop_name} = d_long(iter.token);\n`
                }
                else {
                    struct_vars.push(`  bytes_t ${prop_name}; // ${descr}`)
                    def.impl += `        if (!d_is_bytes(iter.token) && d_type(iter.token) != T_INTEGER && !d_num_bytes(iter.token).data) return rpc_throw(r, "%s.%s must be a numeric value.", "${prop}", "${simple_typename}");\n`
                    def.impl += `        val->${prop_name} = d_num_bytes(iter.token);\n`
                }
                req = `d_type(d_get(ob, key("${prop}"))) == T_NULL`
            }
            else if (pt.type.startsWith("bytes")) {
                const len = parseInt(pt.type.substring(5) || "0")
                struct_vars.push(`  bytes_t ${prop_name}; // ${descr}`)
                if (pt.encoding)
                    def.impl += `        val->${prop_name} = d_bytes_enc(iter.token, ENC_${pt.encoding.toUpperCase()});\n`
                else
                    def.impl += `        val->${prop_name} = d_bytes(iter.token);\n`
                if (len) {
                    def.impl += `        if (${opt_check}!val->${prop_name}.data) return rpc_throw(r, "%s.%s must be bytes ${pt.encoding ? '( encoded as ' + pt.encoding + ')' : ''}!", "${prop}", "${simple_typename}");\n`
                    def.impl += `        if (${opt_check}val->${prop_name}.len != ${len}) return rpc_throw(r, "%s.%s must be exactly %u bytes ${pt.encoding ? '( encoded as ' + pt.encoding + ')' : ''}!", "${prop}", "${simple_typename}", ${len});\n`
                }
                req = `!val->${prop_name}.data`
            }
            else if (pt.type == 'address') {
                struct_vars.push(`  uint8_t* ${prop_name}; // ${descr}`)
                def.impl += `        if (${opt_check}d_bytes(iter.token).len != 20) return rpc_throw(r, "%s.%s must be a valid address with 20 bytes!", "${prop}", "${simple_typename}");\n`
                def.impl += `        val->${prop_name} = d_bytes(iter.token).data;\n`
                req = `!val->${prop_name}`
            }
            else if (pt.type == 'bool') {
                struct_vars.push(`  bool ${prop_name}; // ${descr}`)
                def.impl += `        if (d_type(iter.token) != T_BOOLEAN) return rpc_throw(r, "%s.%s must be a boolean value!", "${prop}", "${simple_typename}");\n`
                def.impl += `        val->${prop_name} = d_int(iter.token);\n`
                req = `d_type(d_get(ob, key("${prop}"))) == T_NULL`
            }
            else if (pt.type == 'string') {
                struct_vars.push(`  char* ${prop_name}; // ${descr}`)
                def.impl += `        val->${prop_name} = d_string(iter.token);\n`
                def.impl += `        if (${opt_check}!val->${prop_name}) return rpc_throw(r, "%s.%s must be a string value ${pt.encoding ? '( encoded as ' + pt.encoding + ')' : ''}!", "${prop}", "${simple_typename}");\n`
                req = `!val->${prop_name}`
            }
            else is_ob = true
        } else
            is_ob = true

        if (is_ob) {
            let ob_name = pt.type
            if (typeof (ob_name) != 'string') ob_name = pt.typeName || prop
            ob_name = 'rpc_' + get_type_name(ob_name) + '_t'
            defineType(ob_name, getType(pt.type, types), types, api, type_defs, prop, { ...init, name: init.name + prop_name + (pt.optional ? '->' : '.') }, pt.type)

            if (pt.optional) {
                const vname = init.name.substring(0, init.name.length - (init.name.endsWith('->') ? 2 : 1)).split('.').join('_').split('->').join('_')
                struct_vars.push(`  ${ob_name}* ${prop_name}; // ${descr}`)
                required += `  if (d_type(d_get(ob, key("${prop}"))) == T_NULL) val->${prop_name} = NULL;\n`
                def.impl += `        if (d_type(iter.token) != T_NULL && ${convert_fn_name(api, ob_name)}(r, iter.token, val->${prop_name})) return rpc_throw(r, "%s->", "${prop}");\n`
                init.vars.push(`${ob_name} ${vname}_${prop_name} = {0}`)
                init.set.push(`${init.name}${prop_name} =  &${vname}_${prop_name}`)
            }
            else {
                struct_vars.push(`  ${ob_name} ${prop_name}; // ${descr}`)
                def.impl += `        if (${convert_fn_name(api, ob_name)}(r, iter.token, &val->${prop_name})) return rpc_throw(r, "%s->", "${prop}");\n`
                req = `d_type(d_get(ob, key("${prop}"))) == T_NULL`
            }
        }
        const val = validate(pt, `val->${prop_name}`, 'r', prop)
        val.check.forEach(_ => def.impl += '        ' + _ + '\n')
        def.impl += `        break;\n`
        if (!pt.optional && req) required += `  if (${req}) return rpc_throw(r, "%s.%s is missing but required!", "${prop}", "${simple_typename}");\n`
    })

    def.header += align_vars(align_vars(struct_vars, '', ' '), '  ', '//').join('\n') + '\n'

    def.impl += `      default: return req_throw_unknown_prop(r, ob, iter.token, "${simple_typename}");\n`
    def.impl += '    }\n  }\n'
    def.impl += required
    def.impl += '  return IN3_OK;\n'
    def.impl += '}'
    def.header += `} ${type_name};\n\n`
    def.header += `/** converts a d_token_t* object to a ${type_name} */\n`
    def.header += `in3_ret_t ${convert_fn_name(api, type_name)}(in3_req_t* r, d_token_t* ob, ${type_name}* val);`

    if (!init.is_include)
        type_defs[type_name] = def

}

function convert_fn_name(api, type_name) {
    const c = type_name.substring(4)
    return 'in3_json_to_' + c.substring(0, c.length - 2);
}

function validate(def, val, req, propname) {
    const res = []

    if (def.enum) {
        const vals = Array.isArray(def.enum) ? def.enum : Object.keys(def.enum)
        res.push('if (' + val + ' && ' + vals.map(_ => `strcmp(${val}, "${_}")`).join(' && ') + ')')
        res.push('  return rpc_throw(' + req + ', "%s must be one of %s", "' + propname + '", "' + vals.join(', ') + '");')
    }

    if (def.validate) def = def.validate
    asArray(def.code).forEach(_ => res.push(_.split('$val').join(val)))

    if (def.array) {
        if (def.min)
            res.push(`if (${def.optional ? val + ' && ' : ''}d_len(${val}) < ${def.min}) return rpc_throw(${req}, "%s must be at least %u items long", "${propname}", ${def.min});`)
        if (def.max)
            res.push(`if (${def.optional ? val + ' && ' : ''}d_len(${val}) > ${def.max}) return rpc_throw(${req}, "%s must be max %u items long", "${propname}", ${def.max});`)
    }
    else if (def.type == 'bytes') {
        if (def.min)
            res.push(`if (${def.optional ? val + '.data && ' : ''}${val}.len < ${def.min}) return rpc_throw(${req}, "%s must be at least ${def.min} bytes long", "${propname}", ${def.min});`)
        if (def.max)
            res.push(`if (${def.optional ? val + '.data && ' : ''}${val}.len > ${def.max}) return rpc_throw(${req}, "%s must be max ${def.max} bytes long", "${propname}", ${def.max});`)
    }
    else if (def.type == 'string') {
        if (def.min)
            res.push(`if (${def.optional ? val + ' && ' : ''}strlen(${val}) < ${def.min}) return rpc_throw(${req}, "%s must be at least ${def.min} characters long", "${propname}", ${def.min});`)
        if (def.max)
            res.push(`if (${def.optional ? val + ' && ' : ''}strlen(${val}) > ${def.max}) return rpc_throw(${req}, "%s must be max ${def.max} characters long", "${propname}", ${def.max});`)
    }
    else if (def.type == 'uint32' || def.type == 'uint64') {
        if (def.min)
            res.push(`if (${val} < ${def.min}) return rpc_throw(${req}, "%s must be at least ${def.min}", "${propname}", ${def.min});`)
        if (def.max)
            res.push(`if (${val} > ${def.max}) return rpc_throw(${req}, "%s must be max ${def.max}", "${propname}", ${def.max});`)
    }

    if (def.type) {
        switch (def.type) {
            case 'url':
                res.push(`if (${def.optional ? val + ' && (' : ''}!strchr(${val},':') || !strchr(${val},'/')${def.optional ? ')' : ''}) return rpc_throw(${req}, "%s must be a valid url", "${propname}");`)
                break
        }
    }



    return { check: res, constants: {} }
}

function getObject(def, name, index, types, api, state) {
    if (!need_structs(state) || def.type == 'any') return {
        args: 'd_token_t* ' + name,
        code_def: 'd_token_t* ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_OBJECT(${name}, ctx, ${index})`
            : `TRY_PARAM_GET_REQUIRED_OBJECT(${name}, ctx, ${index})`,
        code_pass: name,
        validate: validate(def, name, 'ctx->req', name)
    }

    let ob_name = def.typeName || def.type
    if (typeof (ob_name) != 'string') ob_name = name
    ob_name = 'rpc_' + get_type_name(ob_name) + '_t'
    const type_defs = {}
    const type_includes = []
    const dep = resolveTypeDependency(def.type, state)
    if (dep)
        type_includes.push(`#include "${dep}"`)
    let init = { name: name + '.', vars: [], set: [], state, is_include: !!dep }
    defineType(ob_name, getType(def.type, types), types, api, type_defs, ob_name, init, def.type)

    return {
        type_includes,
        type_defs,
        args: ob_name + '* ' + name,
        code_def: ob_name + ' ' + name + ' = {0}' + init.vars.map(_ => ';' + _).join(''),
        code_set: init.set.join(';\n  '),
        code_read: `TRY_PARAM_CONVERT_${def.optional ? '' : 'REQUIRED_'}OBJECT(${name}, ctx, ${index}, ${convert_fn_name(api, ob_name)})`,
        code_pass: def.optional
            ? `d_type(d_get_at(ctx->params, ${index})) == T_NULL ? NULL : &${name}`
            : '&' + name
    }

}

function getArray(def, name, index) {
    return {
        args: 'd_token_t* ' + name,
        code_def: 'd_token_t* ' + name,
        code_read: def.optional
            ? `TRY_PARAM_GET_ARRAY(${name}, ctx, ${index})`
            : `TRY_PARAM_GET_REQUIRED_ARRAY(${name}, ctx, ${index})`,
        code_pass: name,
        validate: validate(def, name, 'ctx->req', name)
    }
}
function need_structs(state) { return typeof (state.generate_rpc) == 'object' && state.generate_rpc.structs !== undefined ? state.generate_rpc.structs : true }
function need_validate(state) { return typeof (state.generate_rpc) == 'object' && state.generate_rpc.validate !== undefined ? state.generate_rpc.validate : true }

function getCType(def, name, index, types, api, state) {
    let c = ''
    if (def.array) return getArray(def, name, index)
    if (typeof (def.type) != 'string') return getObject(def, name, index, types, api, state)
    if (def.type.startsWith("uint") || def.type.startsWith('int')) return getUint(def, name, index)
    if (def.type.startsWith("bytes")) return getBytes(def, name, index)
    switch (def.type) {
        case 'bool': return getBool(def, name, index)
        case 'address': return getAddress(def, name, index)
        case 'string': return getString(def, name, index)
        default: return getObject(def, name, index, types, api, state)
    }
}

function align_vars(src_items, ind, del = ' ', reverse) {
    let items = []
    asArray(src_items).forEach(s => s.split('\n').forEach(_ => items.push(_)))
    if (reverse) items.reverse()
    const none = items.filter(_ => _.indexOf(del) == -1)
    items = items.filter(_ => _.indexOf(del) >= 0)

    let maxl = items.reduce((p, v) => Math.max(p, (v.trim().split(del, 1)[0] || '').length), 0)
    return [...none.map(_ => ind + _.trim()), ...items.map(_ => _.trim()).map(_ => {
        let type = _.split(del, 1)[0] || ''
        const rest = _.substring(type.length + del.length).trim()
        while (type.length < maxl) type += ' '
        return ind + type + del + (del.trim().length ? ' ' : '') + rest
    })]
}



function generate_rpc(path, api_name, rpcs, descr, state) {
    const types = state.types
    let use_conf = false
    let use_main = ''
    try {
        use_main = fs.readFileSync(`${path}/${api_name}.h`, 'utf8')
        use_conf = use_main.indexOf(`${api_name}_config_t`) >= 0;
    } catch (x) { }
    const in3_start = path.indexOf('/in3-core/c/src') > 0 ? path.indexOf('/in3-core/c/src') + '/in3-core/c/src'.length : (path.indexOf('/in3/c/src/') >= 0 ? path.lastIndexOf('/in3/c/src') + '/in3/c/src/'.length : -1)
    const in3_root = in3_start == -1 ? '../../in3/c/src' : relative(path, path.substring(0, in3_start))
    const api_file = api_name
    const rpc_exec = []
    const type_defs = {}
    const conf = use_conf ? `${api_name}_config_t* conf, ` : ''
    api_name = snake_case(api_name)

    const header = [
        compliance_header.join('\n') + '\n',
        comment('', "@file\n" + descr),
        `#ifndef ${api_name}_rpc_h__`,
        `#define ${api_name}_rpc_h__\n`,
        '#ifdef __cplusplus',
        'extern "C" {',
        '#endif\n',

        `#include "${in3_root}/core/client/client.h"`,
        `#include "${in3_root}/core/client/plugin.h"`,

        use_main ? `#include "${api_name}.h"\n` : '',

        comment('', `handles the rpc commands for the ${api_name} modules.`),
        `in3_ret_t ${api_name}_rpc(${conf}in3_rpc_handle_ctx_t* ctx);\n`
    ]

    const impl = [
        compliance_header.join('\n') + '\n',
        `#include "${api_file}_rpc.h"`,
        use_conf ? `#include "${api_file}.h"\n` : '',

        `#include "${in3_root}/core/client/keys.h"`,
        `#include "${in3_root}/core/client/plugin.h"`,
        `#include "${in3_root}/core/client/request_internal.h"`,
        `#include "${in3_root}/core/util/debug.h"`,
        `#include "${in3_root}/core/util/log.h"`,
        `#include "${in3_root}/core/util/mem.h"`,
        `#include "${in3_root}/core/util/utils.h"\n`,
    ]
    const impl_converter_pos = impl.length
    let header_converter_pos = header.findIndex(_ => _ == `#include "${api_file}.h"\n`) + 1 || header.length
    const type_includes = []

    let checked_prefix = null



    Object.keys(rpcs).forEach(rpc_name => {
        if (checked_prefix === null) checked_prefix = rpc_name
        else {
            for (let n = 0; n < checked_prefix.length; n++) {
                if (checked_prefix[n] != rpc_name[n]) {
                    checked_prefix = checked_prefix.substring(0, n)
                    break;
                }
            }
        }
        let prefix = ''

        const r = rpcs[rpc_name];
        const params = []
        const direct_impl = !r.skipGenerate && r._generate_impl
        const code = {
            pre: [],
            read: [],
            pass: [],
            set: [],
            checks: []
        }
        Object.keys(r.params || {}).forEach((p, i) => {
            const t = getCType(r.params[p], p, i, types, api_name, { ...state, path })
            asArray(t.type_includes).forEach(i => type_includes.indexOf(i) < 0 ? type_includes.push(i) : '')
            params.push(t.args)
            t.code_def.split(';').forEach(_ => code.pre.push('  ' + _.trim() + ';'))
            code.read.push('  ' + t.code_read)
            code.pass.push(t.code_pass)
            if (t.code_set) asArray(t.code_set).forEach(_ => code.set.push('  ' + _.trim() + ';'))
            if (t.type_defs)
                Object.keys(t.type_defs).forEach(_ => type_defs[_] = t.type_defs[_])
            if (t.validate && t.validate.check)
                t.validate.check.forEach(_ => code.checks.push(_))
        })
        code.read.push(`  RPC_ASSERT(d_len(ctx->params) <= ${params.length}, "%s only accepts %u arguments.", "${rpc_name}", ${params.length}); `)
        if (r.descr) {
            if (!direct_impl) header.push(comment('', r.descr))
            if (params.length) impl.push(comment('', r.descr))
        }
        if (!direct_impl) header.push(`in3_ret_t ${rpc_name}(${conf}in3_rpc_handle_ctx_t* ctx${params.length ? ', ' + params.join(', ') : ''});\n`)
        if (params.length || direct_impl) {
            if (!direct_impl) prefix = 'handle_'
            if (r.cmakeOptions) impl.push(`#if ${asArray(r.cmakeOptions).map(_ => `defined(${_})`).join(' && ')}`)
            impl.push(`static in3_ret_t ${prefix}${rpc_name}(${conf}in3_rpc_handle_ctx_t* ctx) {`)
            align_vars(align_vars(code.pre, '  '), '  ', '=').forEach(_ => impl.push(_))
            if (code.set.length) impl.push('')
            align_vars(code.set, '  ', '=', true).forEach(_ => impl.push(_))
            impl.push('')
            code.read.forEach(_ => impl.push(_))
            if (code.checks.length) {
                impl.push('')
                code.checks.forEach(_ => impl.push('  ' + _))
            }
            if (direct_impl)
                direct_impl(r, state, type_includes).forEach(_ => impl.push('  ' + _))
            else
                impl.push(`\n  return ${rpc_name}(${use_conf ? 'conf, ' : ''}ctx${params.length ? ', ' + code.pass.join(', ') : ''}); `)
            impl.push('}\n')
            if (r.cmakeOptions) impl.push('#endif')
        }
        if (r.cmakeOptions) {
            if (asArray(r.cmakeOptions).length == 1)
                rpc_exec.push(`#ifdef ${asArray(r.cmakeOptions)[0]}`)
            else
                rpc_exec.push(`#if ${asArray(r.cmakeOptions).map(_ => `defined(${_})`).join(' && ')}`)
        }
        rpc_exec.push(`#if !defined(RPC_ONLY) || defined(RPC_${rpc_name.toUpperCase()})`)
        rpc_exec.push(`  TRY_RPC("${rpc_name}", ${prefix}${rpc_name}(${use_conf ? 'conf, ' : ''}ctx))`)
        rpc_exec.push('#endif\n')
        if (r.cmakeOptions) rpc_exec.push('#endif')
    })

    header.splice(header_converter_pos - 1, 0, ...type_includes)
    header_converter_pos += type_includes.length

    if (Object.keys(type_defs).length) {
        header.splice(header_converter_pos, 0, Object.values(type_defs).map(_ => _.header).join('\n\n') + '\n')
        impl.splice(impl_converter_pos, 0, Object.values(type_defs).map(_ => _.impl).join('\n\n') + '\n')
    }

    header.push('#ifdef __cplusplus')
    header.push('}')
    header.push('#endif\n')
    header.push('#endif')

    //    fs.writeFileSync(path + `/${api_name}_rpc.h`, header.join('\n'), 'utf8')
    state.files[`${path}/${api_file}_rpc.h`] = { lines: header }

    impl.push(comment('', 'handle rpc-requests and delegate execution'));
    impl.push(`in3_ret_t ${api_name}_rpc(${conf}in3_rpc_handle_ctx_t* ctx) {
            `);
    if (checked_prefix) impl.push(`  if (strncmp(ctx->method, "${checked_prefix}", ${checked_prefix.length})) return IN3_EIGNORE; \n`)
    rpc_exec.forEach(_ => impl.push(_))
    if (!checked_prefix || fs.readdirSync(path + '/..').filter(_ => _.startsWith(api_name)).length > 1)
        impl.push(`  return IN3_EIGNORE; `)
    else
        impl.push(`  return rpc_throw(ctx->req, "unknown %s method", "${api_name}"); `)
    impl.push('}')
    state.files[`${path}/${api_file}_rpc.c`] = { lines: impl }

    //    fs.writeFileSync(path + `/${api_name}_rpc.c`, impl.join('\n'), 'utf8')
}
function sort_includes(lines) {
    lines = lines.join('\n').split('\n')
    let includes = lines.filter(_ => _.startsWith('#include '))
    let pos = lines.findIndex(_ => _.startsWith('#include '))
    if (pos >= 0 && includes.length) {
        let a = includes.filter(_ => _.startsWith('#include ".'))
        let b = includes.filter(_ => !_.startsWith('#include ".'))

        a.sort((_a, _b) => _a.localeCompare(_b))
        b.sort((_a, _b) => _a.localeCompare(_b))
        includes = b.length && a.length ? [...b, '', ...a] : [...b, ...a]


        lines = lines.filter(_ => !_.startsWith('#include '))
        lines.splice(pos, 0, ...includes)
    }

    // remove duplicate empty lines
    lines = lines.filter((l, i) => l.trim() || (i == 0 || lines[i - 1].trim()))
    return lines

}
exports.generateAPI = function (api_name, rpcs, descr, types, testCases) {
    let ext = ''
    const typeMapping = {}
    const imports = {}
    if (api_name === 'utils') api_name = 'util'

    // checking for testcases
    if (testCases)

        // iterating through all testcases
        Object.keys(testCases).forEach(tc => createTestCaseFunction(tc, testCases[tc], api_name, rpcs[tc]))

}

exports.generateAllAPIs = function ({ apis, types, conf, cmake_deps, cmake_types }) {
    const all = {}
    const dirs = {}
    apis.forEach(api => {
        dirs[api.conf.api_dir] = [...(dirs[api.conf.api_dir] || []), api]
        Object.keys(api.rpcs).forEach(rpc => {
            const src = api.rpcs[rpc].src
            if (src) {
                const nrpc = all[src] || (all[src] = {})
                nrpc[rpc] = api.rpcs[rpc]
            }
        })
    })
    const files = {}
    Object.keys(all).forEach(path => {
        const p = path.split('/')
        const api = p[p.length - 1].trim()
        generate_rpc(path, p[p.length - 1], all[path], p[p.length - 1] + ' module', { types, cmake_types, cmake_deps, files, generate_rpc: all[path][Object.keys(all[path])[0]].generate_rpc })
    })

    Object.keys(dirs).forEach(dir => {
        let path = ''
        let s = dir.split('/')
        if (s.indexOf('in3') >= 0 && dir.indexOf("in3/c") > 0)
            path = s.slice(s.lastIndexOf('in3') + 3).map(_ => '../').join('') + 'core/client/request_internal.h'
        else
            path = '../../in3/c/src/core/client/request_internal.h'
        const mod = '__RPC_' + dirs[dir][0].api.toUpperCase() + '_H'
        const apis = dirs[dir]

        files[dir + '/rpcs.h'] = {
            lines: [
                ...compliance_header, '// clang-format off', '', `#include "${path}"`, '', '// list of availbale rpc-functions', `#ifndef ${mod}`, `#define ${mod}`, '',
                ...apis.reduce((result, api) => {
                    Object.keys(api.rpcs || []).filter(_ => !api.rpcs[_].alias).forEach(rpc => generate_rpc_define(rpc, api.rpcs[rpc], result))
                    return result
                }, []),
                '#endif']
        }
    })




    Object.keys(files).forEach(file => fs.writeFileSync(file, sort_includes(files[file].lines).join('\n').split('\n').map(l => l.trimEnd()).join('\n'), 'utf8'))
}

function createTest(descr, method, tests, tc) {
    tests.push({
        descr,
        request: { method, params: tc.input || [] },
        result: getResult(tc.expected_output),
        success: tc.expected_failure ? false : true,
        config: tc.config || {},
        response: asArray(tc.mockedResponses)
            .map(r =>
                (r.req.body && r.req.body.params) || r.res.result === undefined ? r.res : r.res.result
            )
    });
}
function getResult(x) {
    let r
    if (x && x.options) {
        r = x.options[type]
    } else if (x && x.type && x.value !== undefined && Object.keys(x).length == 2) {
        if (typeof x.value === 'string') {
            if (x.type.startsWith("uint") && !x.value.startsWith('0x')) return parseInt(x.value)
        }
        return x.value
    } else {
        r = x
    }
    if (Array.isArray(r)) return r.map(getResult)
    return r
}

function createTestCaseFunction(testname, testCase, api_name, rpc) {

    let tests = [];
    if (!rpc) console.log("::: missing rpc-def for " + api_name + ' ' + testname)
    const rpcResult = rpc.result || {}
    asArray(testCase).forEach((t, index) => {
        const tn = (t.descr || testname + (index ? ('_' + (index + 1)) : '')) + (t.extra ? ' : ' + t.extra : '')
        const expected = t && (t.expected_output || t.expected_failure)
        if (rpcResult.options && expected && expected.options) {
            rpcResult.options.forEach(functionDef => {
                const resultType = functionDef.result.array ? functionDef.result.type + "[]" : functionDef.result.type
                const tc = { ...t, input: Array.isArray(t.input) ? [...t.input] : t.input.options[functionDef.name], expected_output: t.expected_output && t.expected_output.options[resultType], mockedResponses: t.mockedResponses.options[resultType] }
                Object.keys(functionDef.params).forEach(prop => {
                    let i = Object.keys(rpc.params).indexOf(prop)
                    if (i < 0) {
                        console.error("Invalid property " + prop + " in " + testname)
                    } else if (functionDef.params[prop].fixed !== undefined) {
                        tc.input = tc.input || []
                        tc.input[i] = functionDef.params[prop].fixed
                    } else if (functionDef.params[prop].optional === false) {
                        // if the param is "required", use the given input
                    } else if (tc.input) {
                        tc.input[i] = functionDef.params[prop]
                    }

                })
                createTest(tn + '_' + functionDef.name, testname, tests, tc)
            })
        } else {
            createTest(tn, testname, tests, t)
        }
    })

    const folders = [
        "../c/test/testdata/requests/generated",
        '../test/requests/generated'
    ]
    const folder = folders.find(f => fs.existsSync(f))

    let fullPath = folder + '/' + (testname.startsWith(api_name + '_') ? '' : (api_name + '_')) + testname + '.json'
    fs.writeFileSync(fullPath, JSON.stringify(tests, null, 2), 'utf8')
}



function generate_rpc_define(rpc, r, content) {
    if (content.find(_ => _.indexOf('#define FN_' + rpc.toUpperCase() + ' "') >= 0)) return
    function get_type(p, def) {
        const ctype = getCType(def, p, 0, {}, {}, { generate_rpc: { structs: false } })
        const t = ctype.args.split(' ')[0].trim()
        switch (t) {
            case 'd_token_t*': return { p, def, t, fmt: '%j', arg: '(d_token_t*) ' + p, conv: p }
            case 'bool': return { p, def, t, fmt: '%i', arg: '(int) ' + p, conv: 'd_int(' + p + ')' }
            case 'bytes_t': return { p, def, t, fmt: '\\"%B\\"', arg: '(bytes_t) ' + p, conv: 'd_bytes(' + p + ')' }
            case 'uint8_t*': return { p, def, t, fmt: '\\"%B\\"', arg: 'bytes(' + p + ', 20)', conv: 'd_bytes(' + p + ').data' }
            case 'uint64_t': return { p, def, t, fmt: '\\"%U\\"', arg: '(uint64_t) ' + p, conv: 'd_long(' + p + ')' }
            case 'char*': return { p, def, t, fmt: '\\"%S\\"', arg: '(char*) ' + p, conv: 'd_string(' + p + ')' }
            default: return { p, def, t: t || 'uint32_t', fmt: '\\"%u\\"', arg: '(uint32_t) ' + p, conv: '(uint32_t) d_long(' + p + ')' }
        }

    }
    const params = Object.keys(r.params || {}).map(p => get_type(p, r.params[p]))
    const rt = get_type('res', r.result || {})
    const use_res = rt.t != 'd_token_t*'
    const aligned = []
    let macro = `\nstatic inline in3_ret_t rpc_call_${rpc}(in3_rpc_handle_ctx_t* ctx, ${rt.t}* _res${params.map(_ => ', ' + _.t + ' ' + _.p).join('')}) {`
    if (!use_res)
        macro = macro.replace('ctx, d_token_t** _res', 'ctx, d_token_t** res')
    else
        aligned.push('d_token_t* res = NULL;')
    if (params.length) {
        aligned.push(`char*      jpayload = sprintx("${params.map(_ => _.fmt).join()}"${params.map(_ => ', ' + _.arg).join('')});`)
        aligned.push(`in3_ret_t  r        = req_send_sub_request(ctx->req, "${rpc}", jpayload, NULL, ${use_res ? '&' : ''}res, NULL);`)
        macro += '\n' + align_vars(align_vars(aligned, '  '), '  ', '=').join('\n')
        macro += `\n  _free(jpayload);`
    }
    else
        macro += '\n' + align_vars(align_vars([...aligned, `in3_ret_t  r = req_send_sub_request(ctx->req, "${rpc}", "", NULL, ${use_res ? '&' : ''}res, NULL);`], '  '), '  ', '=').join('\n')
    if (use_res)
        macro += `\n  if (!r) *_res = ${rt.conv};`
    macro += `\n  return r;`
    macro += `\n}`


    let parameters = params.map(p => `${p.t} ${p.p} :  (${p.def.type}) ${(p.def.descr || p.p).split('\n').join('<brk>')}`)
    parameters = align_vars(parameters, '')
    parameters = align_vars(parameters, '', ':')

    content.push(comment('', (r.descr || camelCaseLow(rpc)) + '\n' + (
        params.length ? (
            '\n\nParameters:\n' + parameters.map(_ => '\n  - ' + _.split('<brk>').join('\n'.padEnd(_.indexOf(':') + 7))).join('')
        ) : '') +
        '\nReturns:\n' + `  - ${rt.t} : (${rt.def.type}) ${(rt.def.descr || 'the result').split('\n').join('\n        ')}`
    )

        + macro
        + '\n#define FN_' + rpc.toUpperCase() + ' "' + rpc + '"\n')

}