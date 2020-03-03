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

#ifndef IN3_STACK_DIAG_H
#define IN3_STACK_DIAG_H

#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t stack_top;
extern size_t stack_size;
extern size_t stack_max_used;

#if PLATFORM_IS_POSIX(PLATFORM)
#include <sys/resource.h>
#define STACK_TOP() STACK_TOP_POSIX()
#else
#define STACK_TOP()
#endif

#define STACK_TOP_POSIX()                                \
  do {                                                   \
    int x_    = 0;                                       \
    stack_top = (size_t) &x_;                            \
    struct rlimit rl;                                    \
    int           result = getrlimit(RLIMIT_STACK, &rl); \
    if (result == 0) {                                   \
      stack_size = rl.rlim_cur;                          \
    }                                                    \
  } while (0)

#define STACK_CURR()                                     \
  do {                                                   \
    int    x_         = 0;                               \
    size_t stack_used = stack_top - (size_t) &x_;        \
    stack_max_used    = max(stack_max_used, stack_used); \
  } while (0)

#define STACK_USED() stack_max_used
#define STACK_SZ() stack_size

#ifdef __cplusplus
}
#endif

#endif //IN3_STACK_DIAG_H
