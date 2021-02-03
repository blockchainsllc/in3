/*******************************************************************************
 * This file is part of the IN3 project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2021 slock.it GmbH, Blockchains LLC
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
const target_platform = process.env.IN3_TARGET || 'index'
const IN3 = require('../in3/' + target_platform + '.js')
const fs = require('fs')
const axios = require('axios')
const Client = IN3
let responses = {}

function hasAPI(api) {
    const c = new IN3()
    const r = !!c[api]
    c.free();
    return r
}

function testTransport(_url, data) {
    try {
        return Promise.resolve(JSON.stringify(JSON.parse(data).map(r => {
            const names = responses[r.method]
            if (!names || !names.length)
                throw new Error('failing to find the request for ' + JSON.stringify(r))
            const rdata = require('../responses/' + r.method + '.json')
            const res = rdata[names[0]]
            if (!res) throw new Error('Could not find response ' + names[0])
            names.splice(0, 1)
            return res
        })))
    }
    catch (x) {
        return Promise.reject(x)
    }
}

function beforeTest(done) {
    responses = {}
    const cache = {}
    IN3.setStorage({
        get: key => cache[key],
        set: (key, value) => cache[key] = value
    })
    IN3.onInit(done).catch(console.error)
}

function mockResponse(method, ...names) {
    const n = responses[method] || (responses[method] = [])
    names.forEach(_ => n.push(_))
}

function createClient(config = {}, recordName) {
    const c = new IN3({
        requestCount: 1,
        autoUpdateList: false,
        proof: 'standard',
        maxAttempts: 1,
        signatureCount: 0,
        nodeRegistry: {
            needsUpdate: false
        },
        ...config
    })
    c.setConfig({ chainId: config.chainId || '0x1' })

    if (recordName)
        IN3.setTransport((url, payload, timeout = 30000) => {
            const req = JSON.parse(payload)[0]
            return axios.post(url, req, { timeout, headers: { 'Content-Type': 'application/json' } })
                .then(res => {
                    if (res.status != 200) throw new Error("Invalid satus")
                    const fname = 'responses/' + req.method + '.json'
                    let data = {}
                    try {
                        data = JSON.parse(fs.readFileSync(fname, 'utf8'))
                    } catch { }
                    data[recordName.shift()] = res.data
                    fs.writeFileSync(fname, JSON.stringify(data, null, 2), 'utf8')
                    return JSON.stringify([res.data])
                })
        })

    else
        IN3.setTransport(testTransport)
    return c
}

module.exports = {
    hasAPI,
    createClient,
    mockResponse,
    IN3,
    beforeTest,
    testTransport
}
