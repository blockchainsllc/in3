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
 * WARRANTY without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
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
const { hasAPI, createClient, mockResponse, IN3, beforeTest } = require('./util/mocker')

let createBtcClient = opts => {
    opts = Object.assign({}, { chainId: '0x99' }, opts)
    return createClient(opts)
}

if (hasAPI('btc'))
    describe('BtcAPI-Tests', () => {
        beforeEach(beforeTest)
        afterEach(IN3.freeAll)

        it('btc.getblockheader()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getblockheader', '0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60')

            let blockHeader = await c.btc.getBlockHeader("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60")

            assert.equal(blockHeader.hash, "0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60")
            assert.equal(blockHeader.confirmations, 13)
            assert.equal(blockHeader.height, 634026)
            assert.equal(blockHeader.version, 1073676288)
            assert.equal(blockHeader.versionHex, "3fff0000")
            assert.equal(blockHeader.merkleroot, "0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1")
            assert.equal(blockHeader.time, 1591782412)
            assert.equal(blockHeader.mediantime, 1591778995)
            assert.equal(blockHeader.nonce, 783981944)
            assert.equal(blockHeader.bits, "17147f35")
            assert.equal(blockHeader.difficulty, 13732352106018.34)
            assert.equal(blockHeader.chainwork, "00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1")
            assert.equal(blockHeader.nTx, 1659)
            assert.equal(blockHeader.previousblockhash, "00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e")
            assert.equal(blockHeader.nextblockhash, "0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb")
        })

        it('btc.getTransaction()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getrawtransaction', '1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d')

            let transaction = await c.btc.getTransaction("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d")

            debugger
            assert.equal(transaction.txid, "1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d")
            assert.equal(transaction.blocktime, 1591782412)
            assert.equal(transaction.confirmations, 17)
            assert.equal(transaction.hash, "371d1b497b6a9930b1f40c30ea18df4027bf452bb312c5945648d85d1f56dad5")
            assert.equal(transaction.hex, "02000000000101adba296f842330296785f061ca9e152dec63fe235d0ef1d4d38cc4a67f586c7c0000000000feffffff0280f0fa020000000017a914e9f20f1225a9528739495649405861ae5d77ba1e871cc9df050000000017a9141abf9a43d3a56f06930b95b1a8f0161bc0b0c9be8702483045022100a1c74e429c4e40ef90d915556ce4b54a9aa4a83872622d0dbbaca3029f07f2d802204d99cd230b2e1d7378401d502cf589227272173f93b3ccc4aed6f97988067e780121035ad17694971c7dadab72369ab2444e355ae7d17ed8fba67aab80da9a3556d37c7cac0900")
            assert.equal(transaction.locktime, 633980)
            assert.equal(transaction.size, 225)
            assert.equal(transaction.time, 1591782412)
            assert.equal(transaction.version, 2)
            assert.equal(transaction.vin.length, 1)

            assert.equal(transaction.vin[0].sequence, 4294967294)
            assert.equal(transaction.vin[0].txinwitness.length, 2)
            assert.equal(transaction.vin[0].vout, 0)
            assert.equal(transaction.vin[0].scriptSig.asm, "")
            assert.equal(transaction.vin[0].scriptSig.hex, "")

            assert.equal(transaction.vout.length, 2)
            debugger
            assert.equal(transaction.vout[0].n, 0)
            assert.equal(transaction.vout[0].value, 0.5)
            assert.equal(transaction.vout[0].scriptPubKey.asm, "OP_HASH160 e9f20f1225a9528739495649405861ae5d77ba1e OP_EQUAL")
            assert.equal(transaction.vout[0].scriptPubKey.hex, "a914e9f20f1225a9528739495649405861ae5d77ba1e87")
            assert.equal(transaction.vout[0].scriptPubKey.addresses.length, 1)

            assert.equal(transaction.vout.length, 2)
            assert.equal(transaction.vsize, 143)
            assert.equal(transaction.weight, 570)
        })

        it('btc.getTransactionBytes()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getrawtransaction', '1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d_bytes')

            let transactionBytes = await c.btc.getTransactionBytes("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d")

            assert.equal(transactionBytes.length, 225)
        })

        it('btc.getBlockWithTxData()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getblock', '0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60')

            let block = await c.btc.getBlockWithTxData("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60")

            assert.equal(block.hash, "0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60")
            assert.equal(block.confirmations, 51)
            assert.equal(block.height, 634026)
            assert.equal(block.version, 1073676288)
            assert.equal(block.versionHex, "3fff0000")
            assert.equal(block.merkleroot, "0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1")
            assert.equal(block.time, 1591782412)
            assert.equal(block.mediantime, 1591778995)
            assert.equal(block.nonce, 783981944)
            assert.equal(block.bits, "17147f35")
            assert.equal(block.difficulty, 13732352106018.34)
            assert.equal(block.chainwork, "00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1")
            assert.equal(block.nTx, 1659)
            assert.equal(block.previousblockhash, "00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e")
            assert.equal(block.nextblockhash, "0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb")
            assert.equal(block.tx.length, 1659)
        })

        it('btc.getBlockWithTxIds()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getblock', '000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22')

            let block = await c.btc.getBlockWithTxIds("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22")

            assert.equal(block.hash, "000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22")
            assert.equal(block.confirmations, 83)
            assert.equal(block.height, 634007)
            assert.equal(block.version, 536870912)
            assert.equal(block.versionHex, "20000000")
            assert.equal(block.merkleroot, "22f78daf63c48b582142421ea17e3a989b1421c1a9d07585668962f19fe12558")
            assert.equal(block.time, 1591770949)
            assert.equal(block.mediantime, 1591768893)
            assert.equal(block.nonce, 3201445374)
            assert.equal(block.bits, "17147f35")
            assert.equal(block.difficulty, 13732352106018.34)
            assert.equal(block.chainwork, "000000000000000000000000000000000000000010380fd08a8436abc886cbc8")
            assert.equal(block.nTx, 131)
            assert.equal(block.previousblockhash, "00000000000000000008c486d4d80a3bd24cc33b0011538baa8d1c3fa7d54c76")
            assert.equal(block.nextblockhash, "00000000000000000001d7fb302a3c72f7cc77d4def5d1d9503195469eb2049f")
            assert.equal(block.tx.length, 131)
        })

        it('btc.getBlockBytes()', async () => {
            const c = createBtcClient({ proof: 'none' })
            mockResponse('getblock', '000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22_bytes')

            let block = await c.btc.getBlockBytes("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22")

            assert.equal(block.length, 55812)
        })
    })