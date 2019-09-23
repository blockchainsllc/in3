
/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

/** @file 
 * logs debug data only if the DEBUG-flag is set.
 * */

#ifdef DEBUG
#define dbg_log(msg, ...) __dbg_log(0, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
#define dbg_log_raw(msg, ...) __dbg_log(1, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
void __dbg_log(int raw, char* file, const char* func, int line, char* fmt, ...);
#else
#define dbg_log(msg, ...)
#define dbg_log_raw(msg, ...)
#endif

#ifdef DBG_FNCTRACE
#define DBG_FNCTRACE_ENTER \
  { printk("ENTER %s::%s\n", __FILE__, __func__); }
#define DBG_FNCTRACE_LEAVE \
  { printk("LEAVE %s::%s\n", __FILE__, __func__); }
#endif //DBG_FNCTRACE

extern void msg_dump(const char* s, unsigned char* data, unsigned len);
