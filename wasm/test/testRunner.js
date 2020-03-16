/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

const fs = require('fs')
const IN3 = require('../../build/bin/in3.js')
const { util } = require('in3-common')
const Client = IN3.default

require('mocha')
const { assert } = require('chai')
const testDir = '../../c/test/testdata/requests'
if (process.argv.find(_ => _.indexOf('mocha') >= 0)) {

    describe('JSON-Tests', () => {

        for (const f of fs.readdirSync(testDir)) {
            it(f, async () => {
                const all = await run_test([testDir + '/' + f], -1)
                for (const r of all)
                    assert.isTrue(r.success, r.c + ' : ' + r.descr + ' failed : ' + r.error)
            })
        }
    })
}

const ignoreFuxxProps = ['id', 'error', 'code', 'weight', 'proofHash', 'registryId', 'timeout', 'lastBlockNumber', 'lastWhiteList', 'currentBlock', 'rpcTime', 'rpcCount', 'gasUsed', 'execTime', 'lastNodeList', 'totalDifficulty', 'size', 'chainId', 'transactionLogIndex', 'logIndex', 'lastValidatorChange']
const ignoreTxProps = ['from', 'blockHash', 'blockNumber', 'publicKey', 'raw', 'standardV', 'transactionIndex']


async function runFuzzTests(filter, test, allResults, c, ob, prefix = '') {
    if (!ob) return c
    for (const k of Object.keys(ob).filter(_ => _ && ignoreFuxxProps.indexOf(_) < 0 && (prefix.indexOf('proof.transactions') < 0 || ignoreTxProps.indexOf(_) < 0))) {
        if (k === 'txIndex' && test.response[0].result === null)
            continue
        const val = ob[k]
        if (typeof val === 'string') {
            if (val.startsWith('0x')) {
                if (val.length == 2) ob[k] = '0x01'
                else if (val[2] === '9') ob[k] = '0xa' + val.substr(3)
                else if (val[2].toLowerCase() === 'f') ob[k] = '0x0' + val.substr(3)
                else ob[k] = '0x' + String.fromCharCode(val[2].charCodeAt(0) + 1) + val.substr(3)
            }
            else continue
        }
        else if (typeof val === 'number')
            ob[k] = val + 1
        else if (Array.isArray(val)) {
            if (val[0] && typeof val[0] === 'object') c = await runFuzzTests(filter, test, allResults, c, val[0], prefix + '.' + k)
            continue
        }
        else if (typeof val === 'object') {
            c = await runFuzzTests(filter, test, allResults, c, val, prefix + '.' + k)
            continue
        }
        else continue

        c++
        if (filter < 0 || c == filter) {
            test.success = false
            const result = await runSingleTest(test, c)
            test.success = true
            allResults.push(result)
            console.log(addSpace('' + result.c, 3) + ' : ' + addSpace('  ' + prefix + '.' + k, 85, '.', result.success ? '' : '31') + ' ' + addSpace(result.success ? 'OK' : result.error, 0, ' ', result.success ? '32' : '31'))
        }
        ob[k] = val
    }

    return c
}



async function run_test(files, filter) {
    const allResults = []
    let c = 0
    for (const file of files) {
        for (const test of JSON.parse(fs.readFileSync(file, 'utf8'))) {
            c++
            if (filter < 0 || c == filter) {
                const result = await runSingleTest(test, c)
                allResults.push(result)
                console.log(addSpace('' + result.c, 3) + ' : ' + addSpace(result.descr, 85, '.', result.success ? '' : '31') + ' ' + addSpace(result.success ? 'OK' : result.error, 0, ' ', result.success ? '32' : '31'))
            }

            if (test.fuzzer)
                c = await runFuzzTests(filter, test, allResults, c, test.response[0])
        }
    }
    return allResults
}

async function runSingleTest(test, c) {
    test = JSON.parse(JSON.stringify(test))
    let res = test.intern ? 1 : 0
    const config = test.config || {}, result = { descr: test.descr, c, success: false, error: undefined }
    let accounts = {}

    Client.setStorage({
        get: _ => {
            if (accounts && _.startsWith('C')) {
                let ac = accounts['0x' + _.substr(1)]
                if (!ac)
                    Object.keys(accounts).forEach(_ => accounts[_.toLowerCase()] = accounts[_])
                ac = accounts['0x' + _.substr(1)]
                if (ac && ac.code)
                    return ac.code.substr(2)
            }
            return null
        }, set: _ => { }
    })
    Client.setTransport((url, data) => {
        /*
        if ((data as any)[0].method == 'in3_validatorlist') {
            const response = JSON.parse(readFileSync(process.cwd() + '/test/util/in3_validatorlist_' + test.chainId + '.json', 'utf8').toString())
            const validatorResponse = mockValidatorList(response, (data as any)[0].params)

            validatorResponse.id = (data as any)[0].id
            validatorResponse.jsonrpc = (data as any)[0].jsonrpc

            return Promise.resolve([validatorResponse])
        }
        */
        if (test.response.length <= res) return Promise.reject(new Error('Not enought responses!'))
        test.response[res].id = data[0].id
        const r = test.response[res++]
        accounts = r.in3 && r.in3.proof && r.in3.proof.accounts
        return Promise.resolve(JSON.stringify([r]))
    })
    const client = new Client({
        requestCount: config.requestCount || 1,
        autoUpdateList: false,
        includeCode: true,
        proof: test.proof || 'standard',
        chainId: test.chainId || '0x1',
        autoUpdateList: false,
        maxAttempts: 1,
        signatureCount: test.signatures ? test.signatures.length : 0,
        nodes: {
            '0x1': {
                needsUpdate: false,
                contract: '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
            },
            '0x5': {
                needsUpdate: false,
                contract: '0x5f51e413581dd76759e9eed51e63d14c8d1379c8',
                registryId: '0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea'
            },
            '0x2a': {
                needsUpdate: false,
                contract: '0x4c396dcf50ac396e5fdea18163251699b5fcca25',
                registryId: '0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf'
            },
            '0x7d0': {
                needsUpdate: false,
                contract: '0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
            }
        }
    })

    let s = false, error = null
    try {
        const response = await client.send(test.request)
        if (test.intern && JSON.stringify(response.result) != JSON.stringify(test.response[0].result))
            throw new Error('wrong result: actual:' + JSON.stringify(response.result) + 'should:' + JSON.stringify(test.response[0].result))
        s = !!(response && response.result !== undefined)
    }
    catch (err) {
        //        console.log(err.stack)
        error = err
    }
    client.free()
    if (s === (test.success == undefined ? true : test.success))
        result.success = true
    else
        result.error = s ? 'Should have failed' : (error && error.message) || 'Failed'
    return result
}

function addSpace(s, l, filler = ' ', color = '') {
    while (s.length < l) s += filler
    return color ? '\x1B[' + color + 'm' + s + '\x1B[0m' : s
}

if (!process.argv.find(_ => _.indexOf('mocha') >= 0)) {
    let filter = -1;
    const files = []
    for (const a of process.argv.slice(2)) {
        if (a == '-t') filter = -2
        else if (filter === -2)
            filter = parseInt(a)
        else if (a.indexOf('.json') != -1)
            files.push(a)
    }
    //console.log("files:", JSON.stringify(files))
    run_test(files, filter).then(() => console.log('done'), console.log)
}

