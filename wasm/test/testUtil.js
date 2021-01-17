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
const { IN3, beforeTest, createClient } = require('./util/mocker')
const convertArray2Hex = a => Array.isArray(a) ? a.map(convertArray2Hex) : IN3.util.toHex(a)
const BN = require('bn.js');

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

    it('randomBytes', async () => {
        let res = IN3.util.randomBytes(40)
        assert.equal(res.byteLength, 40)
    })

    it('soliditySha3', async () => {
        assert.equal(IN3.util.soliditySha3('Hello!%'), "0x661136a4267dba9ccdf6bfddb7c00e714de936674c4bdb065a531cf1cb15c7fc")
        assert.equal(IN3.util.soliditySha3('234'), "0x61c831beab28d67d1bb40b5ae1a11e2757fa842f031a2d0bc94a7867bc5d26c2")
        assert.equal(IN3.util.soliditySha3(0xea), "0x61c831beab28d67d1bb40b5ae1a11e2757fa842f031a2d0bc94a7867bc5d26c2")
        assert.equal(IN3.util.soliditySha3(234n), "0x61c831beab28d67d1bb40b5ae1a11e2757fa842f031a2d0bc94a7867bc5d26c2")
        assert.equal(IN3.util.soliditySha3('234564535', '0xfff23243', true, -10), "0x3e27a893dc40ef8a7f0841d96639de2f58a132be5ae466d40087a2cfa83b7179")
    })

    it('keccak & toHex', async () => {
        const res = IN3.util.keccak("this is just random string")
        assert.equal(IN3.util.toHex(res), "0xdab3b69bd378ba16296c2e116cf7395e352699802234ec4e870b4f4b824248ae")
    })


    it('isAddress', async () => {
        assert.equal(IN3.util.isAddress("0x123"), false)
        assert.equal(IN3.util.isAddress("0x965D1C9987BD2c34e151E63d60AFf8E9dB6b1561"), true)
        assert.equal(IN3.util.isAddress("0x965D1C9987BD2c34e151E63d60AFf8E9dB6b15612"), false)
    })

    it('checkAddressChecksum', async () => {
        assert.equal(IN3.util.checkAddressChecksum("0x123"), false)
        assert.equal(IN3.util.checkAddressChecksum("0x965D1C9987BD2c34e151E63d60AFf8E9dB6b1561"), true)
        assert.equal(IN3.util.checkAddressChecksum("0x965d1C9987BD2c34e151E63d60AFf8E9dB6b1561"), false)
        assert.equal(IN3.util.checkAddressChecksum("0x965D1C9987BD2c34e151E63d60AFf8E9dB6b15612"), false)
    })

    it('getVersion', async () => {
        const res = IN3.util.getVersion()
        assert.match(res, /3\.[0-9]+\.[0-9]+/)
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
        assert.equal('0xffffffffffffffffffffffffffffffff', IN3.util.toHex("340282366920938463463374607431768211455"))
        assert.equal('0xffff', IN3.util.toHex("65535"))
        assert.equal('0xffffffff', IN3.util.toHex("-1", 4))
        assert.equal('0xc384', IN3.util.toHex("Ä"))
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

    describe('setConvertBuffer', () => {
        it('overrides buffer type', () => {
            // Overrides the default function to return a buffer instead of a Uint8Array
            IN3.setConvertBuffer(val => {
                if (val === undefined || val === null || Buffer.isBuffer(val)) {
                    return val;
                }
                if (val instanceof Uint8Array) {
                    return Buffer.from(val);
                }
                if (BN.isBN(val)) {
                    return Buffer.from(val.toString(16), 'hex');
                }
                if (typeof val !== 'string') {
                    val = IN3.util.toHex(val);
                }
                return val.startsWith('0x') ? Buffer.from(val.substr(2), 'hex') : Buffer.from(val, 'utf8');
            });

            let toBeSigned = "some_data"

            const pk = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6"
            const c = createClient()
            c.signer = new IN3.SimpleSigner()
            let address = c.signer.addAccount(pk)
            c.signer.sign(toBeSigned, address).then(signedData => {
                assert.equal(Buffer.isBuffer(signedData), true)
            })
        })

        it('eth.sign()', async () => {
            // Overrides the default function to return a buffer instead of a Uint8Array
            IN3.setConvertBuffer(val => {
                if (val === undefined || val === null || Buffer.isBuffer(val)) {
                    return val;
                }
                if (val instanceof Uint8Array) {
                    return Buffer.from(val);
                }
                if (BN.isBN(val)) {
                    return Buffer.from(val.toString(16), 'hex');
                }
                if (typeof val !== 'string') {
                    val = IN3.util.toHex(val);
                }
                return val.startsWith('0x') ? Buffer.from(val.substr(2), 'hex') : Buffer.from(val, 'utf8');
            });

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
})
