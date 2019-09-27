/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2019 slock.it GmbH, Blockchains LLC
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef SEGGER_RTT
#include "SEGGER_RTT.h"
#define __segger_printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define __segger_vprintf(fmt, args) SEGGER_RTT_vprintf(0, fmt, &args)
#define printX __segger_printf
#define vprintX __segger_vprintf
#elif __ZEPHYR__
#include <zephyr.h>
#define printX printk
#define vprintX vprintk
#else
#define printX printf
#define vprintX vprintf
#endif

void __dbg_log(int raw, char* file, const char* func, int line, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* d = strrchr(file, '/');
  d       = d ? d + 1 : file;

  if (!raw)
    printX("(%s) %s():%d - ", d, func, line);
  vprintX(fmt, args);

  va_end(args);
}

void msg_dump(const char* s, unsigned char* data, unsigned len) {
  unsigned i;

  printX("%s: ", s);
  for (i = 0U; i < len; i++) printX("%02x ", data[i]);
  printX("(%u bytes)\n", len);
}
