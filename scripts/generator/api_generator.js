const fs = require('fs')
const yaml = require('yaml')
const {
    getType,
    asArray,
    camelCaseLow,
    camelCaseUp,
    addAll,
} = require('./util')

const path = require('path');

exports.updateConfig = function (pre, c, key, types) { }

exports.generate_config = function () { }

exports.mergeExamples = function (all) { return all }

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

function createTest(descr, method, tests, tc) {
    const t = {
        descr,
        request: { method, params: tc.input || [] },
        result: getResult(tc.expected_output),
        config: tc.config || {},
        response: []
    }
    tests.push(t);
    (tc.mockedResponses || []).forEach(r => {
        Object.keys(r).forEach(_ => {
            t.response.push(_ == 'in3_http' ? r[_].res[0].result : r[_].res[0])
            if (r[_].length > 1) throw new Error("Too many responses for " + _ + ' in ' + descr)
        })
    })
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
    const rpcResult = rpc.result || {}
    asArray(testCase).forEach((t, index) => {
        const tn = testname + (index ? ('_' + (index + 1)) : '')
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

    let fullPath = '../test/requests/generated/' + (testname.startsWith(api_name + '_') ? '' : (api_name + '_')) + testname + '.json'
    fs.writeFileSync(fullPath, JSON.stringify(tests, null, 2), 'utf8')
}
