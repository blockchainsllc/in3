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