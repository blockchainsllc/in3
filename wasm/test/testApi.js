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

require('mocha')
const { assert } = require('chai')
const { createClient, mockResponse, IN3, beforeTest } = require('./util/mocker')

const contractCode = require('./responses/eth_getCode.json')

describe('API-Tests', () => {
    beforeEach(beforeTest)
    afterEach(IN3.freeAll)

    it('eth.callFn()', async () => {
        const c = createClient()
        mockResponse('eth_call', 'serverData')
        mockResponse('eth_getCode', 'oldRegistry')
        let res = await c.eth.callFn('0x2736D225f85740f42D17987100dc8d58e9e16252', 'servers(uint256):(string,address,uint32,uint256,uint256,address)', 1)
        assert.isArray(res)
        assert.equal(res.length, 6)
        assert.equal(res[0], 'https://in3.slock.it/mainnet/nd-4')
        assert.equal(res[1], '0xbc0ea09c1651a3d5d40bacb4356fb59159a99564')
        assert.equal(res[2], 0xffff)
        assert.equal(res[3], 0xffffn)

        mockResponse('eth_call', 'WETH.totalSupply')
        mockResponse('eth_getCode', 'WETH',)
        res = await c.eth.callFn('0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2', 'totalSupply():uint')
        assert.equal(2156081965638983079156868n, res)


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


    it('eth.sign', async () => {
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

    it('eth.blockNumber()', async () => {
        mockResponse('eth_blockNumber', '0x1')
        const res = await createClient().eth.blockNumber()
        assert.equal(res, 3220)
    })


    it('eth.gasPrice()', async () => {
        mockResponse('eth_gasPrice', 'gas')
        const res = await createClient().eth.gasPrice()
        assert.equal(res, 0xcce416600)
    })

    it('eth.call()', async () => {
        mockResponse('eth_call', 'WETH.totalSupply')
        mockResponse('eth_getCode', 'WETH',)
        const res = await createClient().eth.call({ "to": "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2", "data": "0x18160ddd" })
        assert.equal("0x00000000000000000000000000000000000000000001c8917003f0d0d9d7dc84", res)
    })


    it('eth.estimateGas()', async () => {
        mockResponse('eth_estimateGas', 'WETH.totalSupply')
        const res = await createClient().eth.estimateGas({ "to": "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2", "data": "0x18160ddd" })
        assert.equal(22007, res)
    })


    it('eth.getBalance()', async () => {
        mockResponse('eth_getBalance', 'balance')
        const res = await createClient().eth.getBalance("0x4144FFD5430a8518fa2d84ef5606Fd7e1921cE27")
        assert.equal(res, 3646260000000000000)

    })


    it('eth.getLogs()', async () => {
        mockResponse('eth_getLogs', 'logs')
        const res = await createClient().eth.getLogs({ "fromBlock": "0x834B77", "toBlock": "0x834B77", "address": "0xdac17f958d2ee523a2206206994597c13d831ec7" })
        assert.isArray(res)
        assert.equal(res[0].data, "0x0000000000000000000000000000000000000000000000000000000349d05c5c")
        assert.equal(res[0].transactionHash, "0x20be6d27ed6a4c99c5dbeeb9081e114a9b400c52b80c4d10096c94ad7d3c1af6")
    })

    it('eth.getTransactionReceipt()', async () => {
        mockResponse('eth_getTransactionReceipt', 'receipt')
        const res = await createClient().eth.getTransactionReceipt("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2")
        assert.equal(res.to, "0x5b8174e20996ec743f01d3b55a35dd376429c596")
        assert.equal(res.status, "0x1")
        assert.equal(res.logs[0].address, "0x5b8174e20996ec743f01d3b55a35dd376429c596")

    })

    it('eth.getTransactionByHash()', async () => {
        mockResponse('eth_getTransactionByHash', 'tx')
        const res = await createClient().eth.getTransactionByHash("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2")
        assert.equal(res.blockHash, "0x8220e66456e40636bff3a440832c9f179e4811d4e28269c7ab70142c3e5f9be2")
        assert.equal(res.from, "0x3a9e354dee60df25c0389badafec8457e36ebfd2")

    })

    it('eth.getCode()', async () => {
        mockResponse('eth_getCode', 'codetest')
        const res = await createClient().eth.getCode("0xdAC17F958D2ee523a2206206994597C13D831ec7")
        assert.equal(res, contractCode.codetest.result)

    })

    it('eth.getStorageAt()', async () => {
        mockResponse('eth_getStorageAt', 'storage')
        const res = await createClient().eth.getStorageAt("0x862174623bc39e57de552538f424806b947d3d05", "0x0")
        assert.equal(res, "0x0")

    })


    it('eth.getBlockByHash()', async () => {
        mockResponse('eth_getBlockByHash', 'main')
        const res = await createClient().eth.getBlockByHash("0x0c0728467cee3a0b1b1322e88596d0f9f2a8fa2018fc5447d5072805d3bf9d13")
        assert.equal(res.hash, "0x0c0728467cee3a0b1b1322e88596d0f9f2a8fa2018fc5447d5072805d3bf9d13")
        assert.equal(res.author, "0xea674fdde714fd979de3edf0f56aa9716b898ec8")

    })

    it('eth.getBlockByNumber()', async () => {
        mockResponse('eth_getBlockByNumber', 'main')
        const res = await createClient().eth.getBlockByNumber(0x9d6a46)
        assert.equal(res.hash, "0x0c0728467cee3a0b1b1322e88596d0f9f2a8fa2018fc5447d5072805d3bf9d13")
        assert.equal(res.author, "0xea674fdde714fd979de3edf0f56aa9716b898ec8")

    })


    it('eth.getBlockTransactionCountByHash()', async () => {
        mockResponse('eth_getBlockTransactionCountByHash', 'main')
        const res = await createClient().eth.getBlockTransactionCountByHash("0x0c0728467cee3a0b1b1322e88596d0f9f2a8fa2018fc5447d5072805d3bf9d13")
        assert.equal(0xcc, res)

    })


    it('eth.getBlockTransactionCountByNumber()', async () => {
        mockResponse('eth_getBlockTransactionCountByNumber', 'main')
        const res = await createClient().eth.getBlockTransactionCountByNumber(0x9d6a46)
        assert.equal(0xcc, res)

    })


    it('eth.getTransactionByBlockNumberAndIndex()', async () => {
        mockResponse('eth_getTransactionByBlockNumberAndIndex', 'main')
        const res = await createClient().eth.getTransactionByBlockNumberAndIndex(10317465, 2)
        assert.equal("0xca0e20a9e930af17c7422be3d991e925235b5350f8912a5ca4aaea734b4a88c5", res.blockHash)
        assert.equal('0xea674fdde714fd979de3edf0f56aa9716b898ec8', res.from)

    })


    it('eth.getTransactionByBlockHashAndIndex()', async () => {
        mockResponse('eth_getTransactionByBlockHashAndIndex', 'main')
        const res = await createClient().eth.getTransactionByBlockHashAndIndex('0xca0e20a9e930af17c7422be3d991e925235b5350f8912a5ca4aaea734b4a88c5', 2)
        assert.equal("0xca0e20a9e930af17c7422be3d991e925235b5350f8912a5ca4aaea734b4a88c5", res.blockHash)
        assert.equal('0xea674fdde714fd979de3edf0f56aa9716b898ec8', res.from)

    })

    it('eth.getTransactionCount()', async () => {
        mockResponse('eth_getTransactionCount', 'main')
        const res = await createClient().eth.getTransactionCount('0xea674fdde714fd979de3edf0f56aa9716b898ec8')
        assert.equal(26588838, res)

    })

    it('eth.protocolVersion()', async () => {
        mockResponse('eth_protocolVersion', 'main')
        const res = await createClient().eth.protocolVersion()
        assert.equal('0x40', res)
    })

    it('eth.sendRawTransaction()', async () => {
        mockResponse('eth_sendRawTransaction', 'main')
        const res = await createClient().eth.sendRawTransaction('0xf86d8206aa85438558d400825208941ebd5c8cfec45650cf9c80adfeb5488ae36859c3872386f26fc10000801ba06df6fc901a78cea1dfc02baead56c8ec0d8d008c505efcfc3b4462b1a659be25a065152fd2e0dbf04b8e1baac46f31df9033f9c72b9dfad1b4fe41f89ef32c9a53')
        assert.equal('0x9c8024cc5a409df58f3949848f39c17d1e1d6e53bfe607adac77117fc2724fd2', res)
    })


    it('eth.contractAt()', async () => {
        //        let w = createClient({}, ['weth.Transfer', 'WETH']).eth.contractAt(require('./abi/weth.json'), '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2')



        mockResponse('eth_call', 'weth.name')
        mockResponse('eth_getCode', 'WETH')
        mockResponse('eth_call', 'weth.decimals')
        mockResponse('eth_call', 'weth.balanceOf')
        const weth = createClient().eth.contractAt(require('./abi/weth.json'), '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2')
        assert.equal('Wrapped Ether', await weth.name()) // string
        assert.equal(18, await weth.decimals()) // uint8 -> number
        assert.equal(860298690748n, await weth.balanceOf('0xb958a8f59ac6145851729f73c7a6968311d8b633')) // uint8 -> number

        mockResponse('eth_getLogs', 'weth.Transfer')
        let logs = await weth.events.Transfer.getLogs({ fromBlock: 10317749, toBlock: 10317749 })
        assert.equal(9, logs.length)
        assert.equal('0x7a250d5630b4cf539739df2c5dacb4c659f2488d', logs[0].src)
        assert.equal('0xbb2b8038a1640196fbe3e38816f3e67cba72d940', logs[0].dst)
        assert.equal('Transfer', logs[0].event.event)
        assert.equal(74573366884515930470n, logs[0].wad)
        assert.equal('0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2', logs[0].log.address)


    })


    it('eth.web3ContractAt()', async () => {
        //        let w = createClient({}, ['weth.Transfer', 'WETH']).eth.contractAt(require('./abi/weth.json'), '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2')



        mockResponse('eth_call', 'weth.name')
        mockResponse('eth_getCode', 'WETH')
        mockResponse('eth_call', 'weth.decimals')
        mockResponse('eth_call', 'weth.balanceOf')
        const weth = createClient().eth.web3ContractAt(require('./abi/weth.json'), '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2')
        assert.equal('Wrapped Ether', await weth.methods.name().call()) // string
        assert.equal(18, await weth.methods.decimals().call()) // uint8 -> number
        assert.equal(860298690748n, await weth.methods.balanceOf('0xb958a8f59ac6145851729f73c7a6968311d8b633').call()) // uint8 -> number

        mockResponse('eth_getLogs', 'weth.Transfer')
        let logs = await weth.getPastEvents('Transfer', { fromBlock: 10317749, toBlock: 10317749 })
        assert.equal(9, logs.length)
        assert.equal('0x7a250d5630b4cf539739df2c5dacb4c659f2488d', logs[0].returnValues.src)
        assert.equal('0xbb2b8038a1640196fbe3e38816f3e67cba72d940', logs[0].returnValues.dst)
        assert.equal('Transfer', logs[0].event)
        assert.equal(74573366884515930470n, logs[0].returnValues.wad)
        assert.equal('0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2', logs[0].address)

        /*
        const firstEvent = await new Promise((resolve, reject) => {
            let data = null
            mockResponse('eth_getLogs', 'weth.Transfer')
            const ev = weth.events.Transfer({ fromBlock: 10317749, toBlock: 10317749 })
                .on('error', reject)
                .once('data', resolve)
                .on('test', console.log)
                .off('test', console.log)

            assert.equal(0, ev.listeners.test.length)
            assert.equal(1, ev.listeners.error.length)
            assert.equal(1, ev.listeners.data.length)
        })

        assert.equal('Transfer', firstEvent.event)

*/
    })




    /*
        it('eth.newFilter()', async () => {
            mockResponse('eth_getLogs', 'logs')
            mockResponse('eth_blockNumber', '0x834B77')
            const c = createClient()
            const id = await c.eth.newFilter({ "fromBlock": "0x834B77", "toBlock": "0x834B77", "address": "0xdac17f958d2ee523a2206206994597c13d831ec7" })
            assert.isTrue(id > 0)
    
            const res = await c.eth.getFilterChanges(id)
            assert.isArray(res)
            assert.equal(res[0].data, "0x0000000000000000000000000000000000000000000000000000000349d05c5c")
            assert.equal(res[0].transactionHash, "0x20be6d27ed6a4c99c5dbeeb9081e114a9b400c52b80c4d10096c94ad7d3c1af6")
    
            await c.eth.uninstallFilter(id)
    
    
        })
    */

})