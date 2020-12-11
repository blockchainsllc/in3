/*******************************************************************************
 * This file is part of the IN3 project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2021 slock.it GmbH, Blockchains LLC
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
const { createClient, mockResponse, IN3, beforeTest, hasAPI } = require('./util/mocker')

if (hasAPI('ipfs'))
    describe('IPFS-Tests', () => {
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
