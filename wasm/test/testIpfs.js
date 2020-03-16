require('mocha')
const { assert } = require('chai')
const { createClient, mockResponse, IN3, beforeTest } = require('./util/mocker')

describe('API-Tests', () => {
    beforeEach(beforeTest)
    afterEach(IN3.freeAll)

    it('ipfs_put', async () => {
        mockResponse('ipfs_put', 'multihash')
        const multihash = await createClient({ chainId: '0x7d0', proof: 'none' }).ipfs.put("Lorem ipsum dolor sit amet")
        assert.equal(multihash, 'QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8')
    })

    it('ipfs_get', async () => {
        mockResponse('ipfs_get', 'content')
        const content = await createClient({ chainId: '0x7d0' }).ipfs.get('QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8')
        assert.equal(IN3.util.toUtf8(content), 'Lorem ipsum dolor sit amet')
    })
})