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
const { IN3, beforeTest } = require('./util/mocker')
const convertArray2Hex = a => Array.isArray(a) ? a.map(convertArray2Hex) : IN3.util.toHex(a)

describe('Util-Tests', () => {

    beforeEach(beforeTest)

    afterEach(IN3.freeAll)

    it('abi encode decode', async () => {
        function check(sig, bytes, ...data) {
            assert.equal(
                bytes,
                IN3.util.abiEncode(sig, ...data)
            )
            assert.equal(
                JSON.stringify(convertArray2Hex(data)),
                JSON.stringify(convertArray2Hex(IN3.util.abiDecode('test():' + sig.substr(sig.indexOf('(')), '0x' + bytes.substr(10))))
            )

        }

        check("transfer(address,uint256,string)",
            "0x56b8c724000000000000000000000000965d1c9987bd2c34e151e63d60aff8e9db6b15610000000000000000000000000000000000000000000000000000000000001c4200000000000000000000000000000000000000000000000000000000000000600000000000000000000000000000000000000000000000000000000000000007496e637562656400000000000000000000000000000000000000000000000000",
            "0x965D1C9987BD2c34e151E63d60AFf8E9dB6b1561", 7234, "Incubed"
        )


        check(
            "transfer(bytes,string)",
            "0x8fa7e45f000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000000000000014965d1c9987bd2c34e151e63d60aff8e9db6b15610000000000000000000000000000000000000000000000000000000000000000000000000000000000000007496e637562656400000000000000000000000000000000000000000000000000",
            "0x965D1C9987BD2c34e151E63d60AFf8E9dB6b1561", "Incubed"
        )
        check(
            "transfer(uint8[2])",
            "0xb3168d0e00000000000000000000000000000000000000000000000000000000000000120000000000000000000000000000000000000000000000000000000000000034",
            [0x12, 0x34]
        )
        check(
            "transfer(uint8[])",
            "0x360a10540000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000120000000000000000000000000000000000000000000000000000000000000034",
            [0x12, 0x34]
        )

    })

    it('splitSignature', async () => {

        let res = IN3.util.splitSignature("0xf596af3336ac65b01ff4b9c632bc8af8043f8c11ae4de626c74d834412cb5a234783c14807e20a9e665b3118dec54838bd78488307d9175dd1ff13eeb67e05941c"
            , "0x9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465")

        assert.equal(res.r, "0xf596af3336ac65b01ff4b9c632bc8af8043f8c11ae4de626c74d834412cb5a23")
        assert.equal(res.s, "0x4783c14807e20a9e665b3118dec54838bd78488307d9175dd1ff13eeb67e0594")
        assert.equal(res.v, 28)
        assert.equal(res.messageHash, "0xee966c673733bc2c23275a45f712006c0fecc62eba3660574ac738cc31c7c8b8")

    })

    it('keccak & toHex', async () => {
        const res = IN3.util.keccak("this is just random string")
        assert.equal(IN3.util.toHex(res), "0xdab3b69bd378ba16296c2e116cf7395e352699802234ec4e870b4f4b824248ae")
    })

    it('getVersion', async () => {
        const res = IN3.util.getVersion()
        assert.match(res, /2\.[0-9]+\.[0-9]+/)
    })

    it('getAddress', async () => {
        assert.equal(IN3.util.private2address("0x3f64dd6972bda1e7611dc38a294d7e3404d51c4aff4b09534675ecd43f66d659"),
            "0xeebCfd8F610e497748989B7cbAF0633E644512E6")


    })

    it('toMinHex', async () => {
        const expectedHex = "0x203423"
        assert.equal(IN3.util.toMinHex("0x00000203423"), expectedHex)
        assert.equal(IN3.util.toMinHex("0x0000203423"), expectedHex)

        //assert.equal(IN3.util.toSimpleHex("0x00000203423"),expectedHex)
        assert.equal(IN3.util.toSimpleHex("0x0000203423"), expectedHex)
    })

    it('toHex', async () => {
        assert.equal('0x0a', IN3.util.toHex("0xA"))
        assert.equal('0x01', IN3.util.toHex(1))
        assert.equal(undefined, IN3.util.toHex(undefined))
        assert.equal('0x01', IN3.util.toHex(true))
        assert.equal('0x00', IN3.util.toHex(false))
        assert.equal('0x00000001', IN3.util.toHex(true, 4))
        assert.equal('0xffff', IN3.util.toHex(65535n))
        assert.equal('0x00001234', IN3.util.toHex("0x1234", 4))
        assert.equal('0xff', IN3.util.toHex("255"))
        assert.equal('0x616263', IN3.util.toHex("abc"))
        assert.equal('0x616263', IN3.util.toHex(Buffer.from("abc", 'utf8')))
        assert.Throw(() => IN3.util.toHex({}))
    })


    it('toNumber', async () => {
        assert.equal(1, IN3.util.toNumber(1))
        assert.equal(1, IN3.util.toNumber(true))
        assert.equal(0, IN3.util.toNumber(false))
        assert.equal(65535, IN3.util.toNumber(65535n))
        assert.equal(65535, IN3.util.toNumber("65535"))
        assert.equal(0, IN3.util.toNumber(undefined))
        assert.equal(0, IN3.util.toNumber(null))
        assert.equal(97, IN3.util.toNumber(Buffer.from('a', 'utf8')))
        assert.equal(255, IN3.util.toNumber("0xff"))
        assert.Throw(() => IN3.util.toNumber({}))
    })


    it('toUtf8', async () => {
        const testStr = "this is test"
        let res = IN3.util.toHex(testStr)
        assert.equal(IN3.util.toUtf8(res), testStr)
    })

    it('padding', async () => {
        const testStr = "this is test"

        let res = IN3.util.padStart(testStr, 20)
        assert.equal(res, "        " + testStr)

        let res2 = IN3.util.padEnd(testStr, 20)
        assert.equal(res2, testStr + "        ")
    })

    it('checkSumAddress', async () => {
        assert.equal(IN3.util.toChecksumAddress('0xbc0ea09c1651a3d5d40bacb4356fb59159a99564'), '0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564')
    })


})
