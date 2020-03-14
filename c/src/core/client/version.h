/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

#ifndef IN3_VERSION_H
#define IN3_VERSION_H

#define IN3_COPYRIGHT "2018 - 2018-2020 slock.it, <info@slock.it>."
#define IN3_VERSION_BITS(x, y, z) ((x) << 16 | (y) << 8 | (z))
#define IN3_AT_LEAST_VERSION(x, y, z) (IN3_VERSION_NUM >= IN3_VERSION_BITS(x, y, z))

// Below defines are obtained from cmake
//#define IN3_VERSION "0.0.1"
//#define IN3_VERSION_MAJOR 0
//#define IN3_VERSION_MINOR 0
//#define IN3_VERSION_PATCH 1

#define IN3_VERSION_NUM IN3_VERSION_BITS(IN3_VERSION_MAJOR, IN3_VERSION_MINOR, IN3_VERSION_PATCH)
#ifndef IN3_VERSION
#define IN3_VERSION "local"
#endif

#endif //IN3_VERSION_H
