const IN3 = require('../../../build/bin/in3.js')
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
                needsUpdate: false,
                contract: '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
            },
            '0x5': {
                needsUpdate: false,
                contract: '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
            },
            '0x2a': {
                needsUpdate: false,
                contract: '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
            },
            '0x7d0': {
                needsUpdate: false,
                contract: '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                registryId: '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb'
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