const fs = require('fs')
const yaml = require('../../wasm/test/node_modules/yaml')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    link,
    toCmdParam
} = require('./util')

configs = {
    In3Config: [
        '/// The main Incubed Configuration',
        'public struct In3Config : Codable {',
        '',
        '    /// create a new Incubed Client based on the Configuration',
        '    public func createClient() throws -> In3 {',
        '       return try In3(self)',
        '    }',
        ''
    ]
}
function converterName(swiftType, asFn) {
    const type = swiftType.replace("String:", "").split(/[\[\]_\?\!]+/).join('')
    if (type == 'UInt64' || type == 'Double' || type == 'Bool' || type == 'String' || type == 'Int' || type == 'AnyObject') return 'to' + type
    if (swiftType.startsWith('[') && asFn) {
        if (swiftType.indexOf(':') >= 0) {
            if (swiftType.endsWith('?'))
                return '{ if let dict = try toObject($0,$1) { try dict.mapValues({ try ' + type + '($0,true)! }) } }'
            else
                return '{ try toObject($0,$1)!.mapValues({ try ' + type + '($0,false)! }) }'
        }
        else {
            if (swiftType.endsWith('?'))
                return '{ if let array = try toArray($0,$1) { try array.map({ try ' + type + '($0,true)! }) } }'
            else
                return '{ try toArray($0,$1)!.map({ try ' + type + '($0,false)! }) }'
        }
    }
    return asFn ? '{ try ' + type + '($0,$1) }' : type
}
function generateStruct(swiftType, conf, descr, typeConfigs, typesGenerated, api) {
    typesGenerated[swiftType] = 'placeholder'
    let content = ['/// ' + (descr || swiftType).split('\n').join('\n/// '),
    'public struct ' + swiftType + ' {'
    ]

    let toRPC = '\n    internal func toRPCDict() -> [String:RPCObject] {\n        var obj:[String:RPCObject] = [:]'
    let init = '    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {'
    init += '\n        guard let obj = try toObject(rpc, optional) else { return nil }'


    for (let name of Object.keys(conf)) {
        let p = conf[name]
        const t = getAPIType(p, typeConfigs, typesGenerated, name, api)
        content.push('    /// ' + (p.descr || ('the ' + camelCaseUp(name))).split('\n').join('\n    /// '))
        content.push('    public var ' + name + ': ' + t + '\n')
        if (p.array) {
            if (p.optional) {
                init += '\n        if let ' + name + ' = try toArray(obj["' + name + '"],' + (p.optional ? 'true' : 'false') + ') {'
                init += '\n          self.' + name + ' = try ' + name + '.map({ try ' + converterName(t, false) + '($0,' + (p.optional ? 'true' : 'false') + ')! })'
                init += '\n        } else {'
                init += '\n          self.' + name + ' = nil'
                init += '\n        }'
            }
            else
                init += '\n        ' + name + ' = try toArray(obj["' + name + '"])!.map({ try ' + converterName(t, false) + '($0,' + (p.optional ? 'true' : 'false') + ')! })'
        }
        else if (p.key) {
            if (p.optional) {
                init += '\n        if let ' + name + ' = try toObject(obj["' + name + '"],' + (p.optional ? 'true' : 'false') + ') {'
                init += '\n          self.' + name + ' = try ' + name + '.mapValues({ try ' + converterName(t, false) + '($0,' + (p.optional ? 'true' : 'false') + ')! })'
                init += '\n        } else {'
                init += '\n          self.' + name + ' = nil'
                init += '\n        }'
            }
            else
                init += '\n        ' + name + ' = try toObject(obj["' + name + '"])!.mapValues({ try ' + converterName(t, false) + '($0,false)! })'
        }
        else {
            init += '\n        ' + name + ' = try ' + converterName(t, false) + '(obj["' + name + '"],' + (p.optional ? 'true' : 'false') + ')!'
            if (converterName(t, false).startsWith('to'))
                toRPC += '\n        obj["' + name + '"] = ' + (p.optional ? (name + ' == nil ? RPCObject.none : RPCObject(' + name + '!)') : 'RPCObject(' + name + ')')
        }

    }

    typesGenerated[swiftType] = content.join('\n') + '\n' + init + '\n    }\n' + (toRPC.indexOf('obj["') == -1 ? '' : (toRPC + '\n        return obj\n    }')) + '\n}'
}

function getAPIType(c, typeConfigs, typesGenerated, prefix, api) {
    let swiftType = camelCaseUp(('' + c.type).split('|')[0].trim())
    let typedef = null
    if (typeof c.type === 'object') {
        typedef = c.type
        swiftType = camelCaseUp(api + camelCaseUp(c.typeName || prefix.startsWith('get') ? prefix.substr(3) : prefix))
    }
    else if (typeConfigs[c.type]) {
        typedef = typeConfigs[c.type]
        swiftType = camelCaseUp((c.type.toLowerCase().startsWith(api.toLowerCase()) ? '' : api) + camelCaseUp(c.type))
    }
    else if (swiftType == 'Uint') swiftType = 'UInt64'
    else if (swiftType == 'Float') swiftType = 'Double'
    else if (swiftType == 'Any') swiftType = 'AnyObject'
    else if (swiftType.startsWith('Byte') || swiftType == 'Address' || swiftType == 'Hex') swiftType = 'String'
    if (swiftType.endsWith('[]')) {
        swiftType = swiftType.substr(0, swiftType.length - 2)
        c = { ...c, array: true }
    }
    if (typedef && !typesGenerated[swiftType]) {
        generateStruct(swiftType, typedef, c.descr, typeConfigs, typesGenerated, api);
    }


    if (c.array) swiftType = '[' + swiftType + ']'
    if (c.key) swiftType = '[String:' + swiftType + ']'
    if (c.optional) swiftType += '?'
    return swiftType
}

exports.updateConfig = function (pre, c, key) {
    // generate swift
    const swift = configs[camelCaseUp(pre || 'In3Config')]
    const pad = pre ? '    ' : ''
    if (swift && key.indexOf('-') == -1 && key.indexOf('.') == -1) {
        //console.error("NO Onbject for " + pre + ':Config' + camelCaseUp(pre || ''))
        let swiftType = camelCaseUp(('' + c.type).split('|')[0].trim())
        if (typeof c.type === 'object') {
            swiftType = camelCaseUp(key)
            configs[swiftType] = [
                '    /// ' + c.descr.replace(/\n/gm, '\n/// '),
                '    public struct ' + swiftType + ' : Codable {'
            ]
        }
        else if (swiftType == 'Uint') swiftType = 'UInt64'
        else if (swiftType.startsWith('Byte') || swiftType.startsWith('Address')) swiftType = 'String'
        if (swiftType.endsWith('[]')) swiftType = '[' + swiftType.substr(0, swiftType.length - 2) + ']'
        if (c.array) swiftType = '[' + swiftType + ']'
        swift.push('\n' + pad + '    /// ' + (
            c.descr
            + (c.default ? ('\n(default: `' + JSON.stringify(c.default) + '`)') : '')
            + (c.enum ? ('\n\nPossible Values are:\n\n' + Object.keys(c.enum).map(v => '- `' + v + '` : ' + c.enum[v]).join('\n') + '\n') : '')
            + (c.example ? ('\n\nExample: ' + (Array.isArray(c.example) ? '\n```\n' : '`') + asArray(c.example).map(ex => yaml.stringify(ex).trim()).join('\n') + (Array.isArray(c.example) ? '\n```' : '`')) : '')
        ).replace(/\n/gm, '\n' + pad + '    /// '))
        swift.push(pad + '    public var ' + key + ' : ' + swiftType + (c.optional || !pre ? '?' : ''))
    }
}


function createSwiftInitForStruct(s, pad) {
    let comments = '\n' + pad + '    /// initialize it memberwise'
    let code = ''
    let init = ''
    let lastDescr = ''
    for (let l of s) {
        l = l.trim()
        if (lastDescr && !l) lastDescr = ''
        if (!lastDescr && l.startsWith('/// ')) lastDescr = l.substr(4).split('\n')[0].trim()
        if (l.startsWith('public var ')) {
            l = l.substr('public var '.length)
            const pname = l.substr(0, l.indexOf(':')).trim()
            comments += '\n' + pad + '    /// - Parameter ' + pname + ' : ' + lastDescr
            code += '\n' + pad + '        self.' + pname + ' = ' + pname
            init += (init ? ', ' : '') + l + (l.endsWith('?') ? ' = nil' : '')
            lastDescr = ''
        }
    }
    s.push(comments + '\n' + pad + '    public init(' + init + ') {' + code + '\n' + pad + '    }')
}

exports.generate_config = function () {
    Object.keys(configs).forEach(_ => createSwiftInitForStruct(configs[_], _ == 'In3Config' ? '' : '    '))
    fs.writeFileSync('../swift/Sources/In3/Config.swift', '// This is a generated file, please don\'t edit it manually!\n\nimport Foundation\n\n' + (
        configs.In3Config.join('\n') + '\n\n' +
        Object.keys(configs).filter(_ => _ != 'In3Config').map(type => configs[type].join('\n') + '\n    }\n\n').join('')
        + '\n}\n'
    ), { encoding: 'utf8' })
}

function createApiFunction(rpc_name, rpc, content, api_name, structs, types, rpcs) {
    if (!rpc || rpc.skipApi) return
    if (rpc.alias)
        return createApiFunction(rpc_name, rpcs[rpc.alias], content, api_name, structs, types, rpcs)
    const r = { ...rpc.result, type: (rpc.result || {}).type || 'string' }
    if (r.options) {
        for (option of r.options) {
            let rr = { ...rpc, result: { ...r, ...option.result } }
            if (option.example && rr.example) rr.example = { ...rr.example, response: option.example }
            if (option.params && rr.example && rr.example.request)
                rr.example = { ...rr.example, request: rr.example.request.slice(0, Object.keys(rpc.params).length - Object.keys(option.params).length) }
            rr.params = {}
            for (let pp of Object.keys(rpc.params)) {
                rr.params[pp] = { ...rpc.params[pp] }
                if (option.params[pp] != undefined) rr.params[pp].fixed = option.params[pp]
            }
            delete rr.result.options
            rr.apiName = option.name || rr.apiName
            rr.descr = option.descr || rr.descr
            createApiFunction(rpc_name, rr, content, api_name, structs, types, rpcs)
        }
        return
    }

    if (rpc.descr) content.push('    /// ' + rpc.descr.split('\n').join('\n    /// '))
    else content.push('    /// ' + rpc_name)
    const fnName = rpc.apiName || camelCaseLow(rpc_name.substr(rpc_name.indexOf('_') + 1))
    let s = '    public func ' + fnName + '('
    let params = ''
    if (rpc.params) {
        for (let name of Object.keys(rpc.params)) {
            let p = rpc.params[name]
            let type = getAPIType(p, types, structs, name, camelCaseUp(api_name))
            if (p.fixed === undefined) {
                if (!s.endsWith('(')) s += ', '
                content.push('    /// - Parameter ' + name + ' : ' + (p.descr || name).split('\n').join('    /// '))
                s += name + ': ' + type + (p.optional || p.default !== undefined ? ' = ' + (p.default !== undefined ? JSON.stringify(p.default) : 'nil') : '')
            }
            params += (params ? '' : 'params:') + (
                p.fixed === undefined
                    ? (p.optional
                        ? name + ' == nil ? RPCObject.none : RPCObject(' + name + '!),'
                        : ' RPCObject(' + (converterName(type, false).startsWith('to') ? name : (name + '.toRPCDict()')) + '),')
                    : (' RPCObject(' + JSON.stringify(p.fixed) + '), ')
            )

        }
    }
    const returnType = getAPIType(r, types, structs, fnName, camelCaseUp(api_name))
    if (rpc.sync) {
        s += ') throws ->  ' + returnType.replace("AnyObject", "RPCObject") + ' {'
        s += '\n        return try execLocalAndConvert(in3: in3, method: "' + rpc_name + '",' + params
        if (returnType == '[AnyObject]')
            s += ' convertWith: { try toArray($0,$1)! } )'
        else
            s += ' convertWith: ' + converterName(returnType, true) + ' )'
        s += '\n    }\n'
    }
    else {
        s += ') -> Future<' + returnType + '> {'
        s += '\n        return execAndConvert' + (r.optional ? 'Optional' : '') + '(in3: in3, method: "' + rpc_name + '",' + params
        s += ' convertWith: ' + converterName(returnType, true) + ' )'
        s += '\n    }\n'
    }
    if (r.descr) content.push('    /// - Returns: ' + rpc.result.descr.split('\n').join('\n    /// '))

    asArray(rpc.example).forEach(ex => {
        const paramNames = Object.keys(rpc.params || {})
        let x = '\n**Example**\n\n```swift\n'
        let call = camelCaseUp(api_name) + 'API(in3).' + fnName + '(' + (ex.request || []).map((_, i) => paramNames[i] + ': ' + JSON.stringify(_)).join(', ') + ')'
        if (rpc.sync) {
            x += 'let result = try ' + call + '\n'
            x += '// result = ' + (typeof ex.response === 'object' ? '\n//          ' : '') + yaml.stringify(ex.response).trim().split('\n').join('\n//          ')
        }
        else {
            x += call + ' .observe(using: {\n'
            x += '    switch $0 {\n'
            x += '       case let .failure(err):\n'
            x += '         print("Failed because : \\(err.localizedDescription)")\n'
            x += '       case let .success(val):\n'
            x += '         print("result : \\(val)")\n'
            x += '//              result = ' + (typeof ex.response === 'object' ? '\n//          ' : '') + yaml.stringify(ex.response).trim().split('\n').join('\n//          ')
            x += '\n     }\n'
            x += '}\n'
        }
        x += '\n```\n'
        content.push('    /// ' + x.split('\n').join('\n    /// '))
    })
    content.push(s)
}

exports.generateAPI = function (api_name, rpcs, descr, types) {
    const structs = {}
    const apiName = camelCaseUp(api_name) + 'API'
    const content = [
        '/// this is generated file don\'t edit it manually!',
        '',
        'import Foundation',
        '',
        '/// ' + descr.split('\n').join('\n/// '),
        'public class ' + apiName + ' {',
        '    internal var in3: In3',
        '',
        '    /// initialiazes the ' + camelCaseUp(api_name) + ' API',
        '    /// - Parameter in3 : the incubed Client',
        '    init(_ in3: In3) {',
        '       self.in3 = in3',
        '    }',
        ''
    ]
    Object.keys(rpcs).forEach(rpc_name => createApiFunction(rpc_name, rpcs[rpc_name], content, api_name, structs, types, rpcs))

    fs.writeFileSync('../swift/Sources/In3/API/' + apiName + '.swift', (
        content.join('\n') + '\n\n}\n' +
        Object.values(structs).join('\n\n')
    ), 'utf8')
}



function generate_swift_api(all) {
    Object.keys(all).forEach(api_key => {
        const api = all[api_key]

    })
}
