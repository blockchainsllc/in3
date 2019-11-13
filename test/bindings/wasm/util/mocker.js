const IN3 = require('../../../../build/bin/in3.js')
const Client = IN3
let responses = {}
function test_transport(url, data) {
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

function beforeTest(done) {
    responses = {}
    IN3.onInit(done).catch(console.error)
}

function mockResponse(method, ...names) {
    const n = responses[method] || (responses[method] = [])
    names.forEach(_ => n.push(_))
}

function createClient(config = {}) {
    const c = new IN3({
        requestCount: 1,
        autoUpdateList: false,
        proof: 'standard',
        chainId: '0x1',
        autoUpdateList: false,
        maxAttempts: 1,
        signatureCount: 0,
        nodes: {
            '0x1': {
                needsUpdate: false
            },
            '0x5': {
                needsUpdate: false
            },
            '0x2a': {
                needsUpdate: false
            }
        },
        ...config
    })
    IN3.setTransport(test_transport)
    return c
}

module.exports = {
    createClient,
    mockResponse,
    IN3,
    beforeTest

}