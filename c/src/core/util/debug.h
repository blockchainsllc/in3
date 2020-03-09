
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

/** @file 
 * logs debug data only if the DEBUG-flag is set.
 * */

#ifndef IN3_EXPORT_TEST
#define IN3_EXPORT_TEST static
#endif

#ifdef DEBUG
#define dbg_log(msg, ...) __dbg_log(0, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
#define dbg_log_raw(msg, ...) __dbg_log(1, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
void __dbg_log(int raw, char* file, const char* func, int line, char* fmt, ...);
#else
/** logs a debug-message including file and linenumber*/
#define dbg_log(msg, ...)
/** logs a debug-message without the  filename */
#define dbg_log_raw(msg, ...)
#endif

#ifdef DBG_FNCTRACE
#define DBG_FNCTRACE_ENTER \
  { printk("ENTER %s::%s\n", __FILE__, __func__); }
#define DBG_FNCTRACE_LEAVE \
  { printk("LEAVE %s::%s\n", __FILE__, __func__); }
#endif //DBG_FNCTRACE

/** dumps the given data as hex coded bytes to stdout */
extern void msg_dump(const char* s, const unsigned char* data, unsigned len);
