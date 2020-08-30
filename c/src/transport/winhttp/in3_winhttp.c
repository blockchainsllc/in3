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

#include "in3_winhttp.h"
#include "../../core/client/client.h"
#include "../../core/client/plugin.h"
#include "../../core/client/version.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include <stdio.h>  /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <string.h> /* memcpy, memset */
#include <time.h>
#include <unistd.h> /* read, write, close */
#include <windows.h>
#include <winhttp.h>

static inline wchar_t* convert_wstr(const char* src, void* dst) {
  MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, (int) MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0));
  return dst;
}
#define to_wstr(c) convert_wstr(c, alloca(MultiByteToWideChar(CP_UTF8, 0, c, -1, NULL, 0) * sizeof(wchar_t) + 1))

in3_ret_t send_winhttp(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_request_t* req = plugin_ctx;
  if (action != PLGN_ACT_TRANSPORT_SEND) return IN3_ENOTSUP;
  for (unsigned int n = 0; n < req->urls_len; n++) {
    uint32_t  start    = current_ms();
    sb_t*     sb       = &req->ctx->raw_response[n].data;
    HINTERNET hSession = WinHttpOpen(to_wstr("in3 winhttp " IN3_VERSION),
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
      in3_ctx_add_response(req->ctx, n, true, "could not create the session", -1, 0);
      continue;
    }

    URL_COMPONENTS url_components;
    wchar_t        hostname[128];
    wchar_t        url_path[4096];
    wchar_t*       url = to_wstr(req->urls[n]);
    memset(&url_components, 0, sizeof(URL_COMPONENTS));
    url_components.dwStructSize     = sizeof(URL_COMPONENTS);
    url_components.lpszHostName     = hostname;
    url_components.dwHostNameLength = 128;
    url_components.lpszUrlPath      = url_path;
    url_components.dwUrlPathLength  = 1024;

    WinHttpCrackUrl(url, 0, 0, &url_components);

    HINTERNET connect = WinHttpConnect(hSession, url_components.lpszHostName, url_components.nPort, 0);
    if (!connect) {
      in3_ctx_add_response(req->ctx, n, true, "could not connect to ", -1, 0);
      sb_add_chars(sb, req->urls[n]);
      sb_add_chars(sb, "Error code : ");
      sb_add_int(sb, GetLastError());
      WinHttpCloseHandle(hSession);
      continue;
    }
    bool      https   = strstr(req->urls[n], "https") == req->urls[n];
    HINTERNET request = WinHttpOpenRequest(connect, to_wstr("POST"),
                                           url_components.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, https ? WINHTTP_FLAG_SECURE : 0);
    if (!request) {
      in3_ctx_add_response(req->ctx, n, true, "could not open the request to ", -1, 0);
      sb_add_chars(sb, req->urls[n]);
      sb_add_chars(sb, "Error code : ");
      sb_add_int(sb, GetLastError());
      WinHttpCloseHandle(connect);
      WinHttpCloseHandle(hSession);
      continue;
    }
    int  plen    = strlen(req->payload);
    bool success = plen
                       ? WinHttpSendRequest(
                             request,
                             to_wstr("Accept: application/json\r\nContent-Type: application/json\r\ncharsets: utf-8\r\n"),
                             (DWORD) -1,
                             (LPVOID) req->payload,
                             (DWORD) plen, (DWORD) plen, 0)
                       : WinHttpSendRequest(
                             request,
                             WINHTTP_NO_ADDITIONAL_HEADERS,
                             0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (success) {
      WinHttpReceiveResponse(request, NULL);
      DWORD dwSize       = 0;
      DWORD dwDownloaded = 0;
      LPSTR pszOutBuffer = NULL;

      do {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(request, &dwSize)) {
          sb->len = 0;
          in3_ctx_add_response(req->ctx, n, true, "could not read the data from ", -1, 0);
          sb_add_chars(sb, req->urls[n]);
          sb_add_chars(sb, "Error code : ");
          sb_add_int(sb, GetLastError());
          break;
        }

        // No more available data.
        if (!dwSize)
          break;

        // Allocate space for the buffer.
        pszOutBuffer = _calloc(dwSize + 1, 1);

        if (!WinHttpReadData(request, (LPVOID) pszOutBuffer,
                             dwSize, &dwDownloaded)) {
          sb->len = 0;
          in3_ctx_add_response(req->ctx, n, true, "could not read the data from ", -1, 0);
          sb_add_chars(sb, req->urls[n]);
          sb_add_chars(sb, "Error code : ");
          sb_add_int(sb, GetLastError());
          _free(pszOutBuffer);
          break;
        }
        else
          sb_add_range(sb, pszOutBuffer, 0, dwSize);

        // Free the memory allocated to the buffer.
        _free(pszOutBuffer);

        // This condition should never be reached since WinHttpQueryDataAvailable
        // reported that there are bits to read.
        if (!dwDownloaded)
          break;

      } while (dwSize > 0);

      if (req->ctx->raw_response[n].state == IN3_WAITING) {
        req->ctx->raw_response[n].state = IN3_OK;
        req->ctx->raw_response[n].time  = current_ms() - start;
      }
    }
    else {
      in3_ctx_add_response(req->ctx, n, true, "could not open send the request to ", -1, 0);
      sb_add_chars(sb, req->urls[n]);
      sb_add_chars(sb, "Error code : ");
      sb_add_int(sb, GetLastError());
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(hSession);
  }
  return IN3_OK;
}

in3_ret_t in3_register_winhttp(in3_t* c) {
  return plugin_register(c, PLGN_ACT_TRANSPORT, send_winhttp, NULL, true);
}
