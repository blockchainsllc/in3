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

const { IN3 } = require('./util/mocker')
const { keccak256 } = require('eth-lib/lib/hash')

function testHash(fn, l = 1000) {
    const start = Date.now()
    let init = "0x1234567890123456789012345678901234567890123456789012345678901234"
    for (let i = 0; i < l; i++) {
        init = fn(init);
    }
    const s = Date.now() - start
    return s
}
let j1 = 0, j2 = 0, w1 = 0, w2 = 0
const l2 = 100000
console.log("eth-lib1:", j1 = testHash(keccak256))
console.log("eth-lib2:", j2 = testHash(keccak256, l2))
IN3.onInit(() => {
    console.log("in3-wasm1:", w1 = testHash(IN3.util.keccak))
    console.log("in3-wasm2:", w2 = testHash(IN3.util.keccak, l2))

    console.log("1 : " + (j1 / w1).toFixed(1) + ' times faster ')
    console.log("2 : " + (j2 / w2).toFixed(1) + ' times faster ')
})
