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

    it('checkSumAddress', async () => {
        IN3.onInit(() => {
            assert.equal(IN3.util.toChecksumAddress('0xbc0ea09c1651a3d5d40bacb4356fb59159a99564'), '0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564')
        })
    })


    it('eth_callFn', async () => {
        mockResponse('eth_call', 'serverData')
        const res = await createClient().eth.callFn('0x2736D225f85740f42D17987100dc8d58e9e16252', 'servers(uint256):(string,address,uint32,uint256,uint256,address)', 1)
        assert.isArray(res)
        assert.equal(res.length, 6)
        assert.equal(res[0], 'https://in3.slock.it/mainnet/nd-4')
        assert.equal(res[1], '0xbc0ea09c1651a3d5d40bacb4356fb59159a99564')
        assert.equal(res[2], 0xffff)
        assert.equal(res[3], 0xffffn)
    })
})


