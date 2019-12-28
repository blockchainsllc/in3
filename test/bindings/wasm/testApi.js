/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

require('mocha')
const { assert } = require('chai')
const { createClient, mockResponse, IN3, beforeTest } = require('./util/mocker')



describe('API-Tests', () => {
    beforeEach(beforeTest)
    afterEach(IN3.freeAll)
    it('checkSumAddress', async () => {
        assert.equal(IN3.util.toChecksumAddress('0xbc0ea09c1651a3d5d40bacb4356fb59159a99564'), '0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564')
    })

    it('eth_callFn', async () => {
        mockResponse('eth_call', 'serverData')
        mockResponse('eth_getCode', 'oldRegistry')
        const res = await createClient().eth.callFn('0x2736D225f85740f42D17987100dc8d58e9e16252', 'servers(uint256):(string,address,uint32,uint256,uint256,address)', 1)
        assert.isArray(res)
        assert.equal(res.length, 6)
        assert.equal(res[0], 'https://in3.slock.it/mainnet/nd-4')
        assert.equal(res[1], '0xbc0ea09c1651a3d5d40bacb4356fb59159a99564')
        assert.equal(res[2], 0xffff)
        assert.equal(res[3], 0xffffn)
    })
    it('send_transaction', async () => {
        mockResponse('eth_gasPrice', 'default')
        mockResponse('eth_estimateGas', '1M')
        mockResponse('eth_getTransactionCount', 'default')
        mockResponse('eth_sendRawTransaction', 'test_hash')

        const pk = '0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6'
        const address = IN3.util.private2address(pk)
        const c = createClient({ proof: 'none' })
        c.signer = new IN3.SimpleSigner(pk)

        const hash = await c.eth.sendTransaction({
            from: address,
            to: '0x1234567890123456789012345678901234567890',
            method: 'setData(uint256,string)',
            args: [123, 'testdata'],
            confirmations: 0
        })

        assert.equal(hash, '0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38')

    })


    it('sign', async () => {
        const pk = '0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6'
        const msg = '0x9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465'
        assert.equal(IN3.util.toHex(IN3.util.ecSign(pk, msg, false))
            , '0xf596af3336ac65b01ff4b9c632bc8af8043f8c11ae4de626c74d834412cb5a234783c14807e20a9e665b3118dec54838bd78488307d9175dd1ff13eeb67e05941c')
        assert.equal(IN3.util.toHex(IN3.util.ecSign(pk, msg, true))
            , '0x349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c')

        const address = IN3.util.private2address(pk)
        assert.equal(address, '0x082977959d0C5A1bA627720ac753Ec2ADB5Bd7d0')


        const c = createClient()
        assert.isTrue(await c.eth.sign(address, msg).then(_ => false, _ => true), 'must throw since we don not have a signer set')

        c.signer = new IN3.SimpleSigner(pk)
        const sig = await c.eth.sign(address, msg)
        assert.equal(sig.message, '0x19457468657265756d205369676e6564204d6573736167653a0a33329fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465')
        assert.equal(sig.messageHash, IN3.util.toHex(IN3.util.keccak(sig.message)))
        assert.equal(sig.signature, '0x5782d5df271b9a0890f89868de73b7a206f2eb988346bc3df2c0a475d60b068a30760b12fd8cf88cd10a31dea71d9309d5b7b2f7bb49e36f69fcdbdfe480f1291c')
        assert.equal(sig.r, '0x5782d5df271b9a0890f89868de73b7a206f2eb988346bc3df2c0a475d60b068a')
        assert.equal(sig.s, '0x30760b12fd8cf88cd10a31dea71d9309d5b7b2f7bb49e36f69fcdbdfe480f129')
        assert.equal(sig.v, 28)

    })



})


