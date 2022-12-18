/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef SEGGER_RTT
#include "SEGGER_RTT.h"
#define __segger_printf(...)        SEGGER_RTT_printf(0, __VA_ARGS__)
#define __segger_vprintf(fmt, args) SEGGER_RTT_vprintf(0, fmt, &args)
#define printX                      __segger_printf
#define vprintX                     __segger_vprintf
#elif __ZEPHYR__
#include <zephyr.h>
#define printX  printk
#define vprintX vprintk
#else
#define printX  printf
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

/**
 * converts a error code into a string.
 */
char* in3_errmsg(in3_ret_t err /**< the error code */) {
#ifdef LOGGING
  switch (err) {
    case IN3_OK: return NULL;
    case IN3_ECONFIG: return "Invalid configuration";
    case IN3_EFIND: return "Could not find the requested resource";
    case IN3_EUNKNOWN: return "Unknown Error occured";
    case IN3_ENOMEM: return "Out of Memory";
    case IN3_ENOTSUP: return "The operation is not supported";
    case IN3_EINVAL: return "Invalid Value";
    case IN3_EVERS: return "Version missmatched";
    case IN3_ELIMIT: return "Limit reached";
    case IN3_EINVALDT: return "invalid data";
    case IN3_EPASS: return "wrong password";
    case IN3_ERPC: return "RPC Error";
    case IN3_ERPCNRES: return "RPC No response";
    case IN3_EUSNURL: return "RPC invalid url";
    case IN3_ETRANS: return "transport error";
    case IN3_ERANGE: return "out of range";
    case IN3_WAITING: return "waiting for data";
    case IN3_EIGNORE: return "ignoreable error";
    case IN3_EPAYMENT_REQUIRED: return "payment required";
    case IN3_ENODEVICE: return "no hardware wallet connected";
    case IN3_EAPDU: return "error in usb communication protocol";
    case IN3_EPLGN_NONE: return "no plugin could handle specified action";
    case IN3_ERETRY: return "request to retry all plugins";
    case IN3_EINCOMPLETE: return "Data are not complete yet";
    case IN3_HTTP_BAD_REQUEST: return "400 - Bad Request";
    case IN3_HTTP_UNAUTHORIZED: return "401 - Unauthorized";
    case IN3_HTTP_PAYMENT_REQUIRED: return "402 - Payment required";
    case IN3_HTTP_FORBIDDEN: return "403 - Forbidden";
    case IN3_HTTP_NOT_FOUND: return "404 - Not found";
    case IN3_HTTP_M_NOT_ALLOWED: return "405 - Method not allowed";
    case IN3_HTTP_NOT_ACCEPTABLE: return "406 - Not acceptable";
    case IN3_HTTP_PROX_AUTH_REQUIRED: return "407 - Proxy Authetification required";
    case IN3_HTTP_TIMEOUT: return "408 - Request timeout";
    case IN3_HTTP_CONFLICT: return "409 - Conclict";
    case IN3_HTTP_GONE: return "410 - Gone";
    case IN3_HTTP_INTERNAL_ERROR: return "500 - Internal Server Error";
    case IN3_HTTP_NOT_IMPLEMENTED: return "501 - Not Implemented";
    case IN3_HTTP_BAD_GATEWAY: return "502 - Bad Gateway";
    case IN3_HTTP_UNAVAILABLE: return "503 - Service Unavailable";
  }
  return NULL;
#else
  return err ? "E" : NULL;
#endif
}
