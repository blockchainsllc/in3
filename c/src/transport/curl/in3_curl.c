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

#include "in3_curl.h"
#include "../../core/client/client.h"
#include "../../core/client/version.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include <curl/curl.h>
#include <string.h>

#ifndef CURL_MAX_PARALLEL
#define CURL_MAX_PARALLEL 50
#endif

/*
struct MemoryStruct {
  char *memory = NULL;
  size_t size;
};
 */
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  in3_response_t* r = (in3_response_t*) userp;
  sb_add_range(&r->result, contents, 0, size * nmemb);
  return size * nmemb;
}

static void readDataNonBlocking(CURLM* cm, const char* url, const char* payload, struct curl_slist* headers, in3_response_t* r, uint32_t timeout) {
  CURL*     curl;
  CURLMcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(payload));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (uint64_t) timeout / 1000L);

    /* Perform the request, res will get the return code */
    res = curl_multi_add_handle(cm, curl);
    if (res != CURLM_OK) {
      sb_add_chars(&r->error, "curl_multi_add_handle() failed:");
      sb_add_chars(&r->error, (char*) curl_easy_strerror(res));
    }
  } else
    sb_add_chars(&r->error, "no curl:");
}

in3_ret_t send_curl_nonblocking(const char** urls, int urls_len, char* payload, in3_response_t* result, uint32_t timeout) {
  CURLM*   cm;
  CURLMsg* msg;
  int      transfers   = 0;
  int      msgs_left   = -1;
  int      still_alive = 1;

  cm = curl_multi_init();
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long) CURL_MAX_PARALLEL);

  struct curl_slist* headers = NULL;
  headers                    = curl_slist_append(headers, "Accept: application/json");
  headers                    = curl_slist_append(headers, "Content-Type: application/json");
  headers                    = curl_slist_append(headers, "charsets: utf-8");
  headers                    = curl_slist_append(headers, "User-Agent: in3 curl " IN3_VERSION);
  for (transfers = 0; transfers < min(CURL_MAX_PARALLEL, urls_len); transfers++)
    readDataNonBlocking(cm, urls[transfers], payload, headers, result + transfers, timeout);

  do {
    curl_multi_perform(cm, &still_alive);

    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
      if (msg->msg == CURLMSG_DONE) {
        char* url;
        CURL* e = msg->easy_handle;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
        // fprintf(stderr, "R: %d - %s <%s>\n",
        //        msg->data.result, curl_easy_strerror(msg->data.result), url);
        curl_multi_remove_handle(cm, e);
        curl_easy_cleanup(e);
      } else {
        // fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
        sb_add_chars(&result->error, "E: CURLMsg");
      }
      if (transfers < urls_len) {
        readDataNonBlocking(cm, urls[transfers], payload, headers, result + transfers, timeout);
        transfers++;
      }
    }

    if (still_alive)
      curl_multi_wait(cm, NULL, 0, 1000, NULL);

  } while (still_alive || (transfers < urls_len));

  curl_slist_free_all(headers);
  curl_multi_cleanup(cm);

  for (int i = 0; i < urls_len; i++) {
    if ((result + i)->error.len) {
      in3_log_debug("curl: failed for %s\n", urls[i]);
      return IN3_ETRANS; // return error if even one failed
    }
  }
  return IN3_OK;
}

static void readDataBlocking(const char* url, char* payload, in3_response_t* r, uint32_t timeout) {
  CURL*    curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(payload));

    struct curl_slist* headers = NULL;
    headers                    = curl_slist_append(headers, "Accept: application/json");
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    headers                    = curl_slist_append(headers, "charsets: utf-8");
    headers                    = curl_slist_append(headers, "User-Agent: in3 curl " IN3_VERSION);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (uint64_t) timeout / 1000L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      sb_add_chars(&r->error, "curl_easy_perform() failed:");
      sb_add_chars(&r->error, (char*) curl_easy_strerror(res));
    }
    curl_slist_free_all(headers);
    /* always cleanup */
    curl_easy_cleanup(curl);
  } else
    sb_add_chars(&r->error, "no curl:");
}

in3_ret_t send_curl_blocking(const char** urls, int urls_len, char* payload, in3_response_t* result, uint32_t timeout) {
  int i;
  for (i = 0; i < urls_len; i++)
    readDataBlocking(urls[i], payload, result + i, timeout);
  for (i = 0; i < urls_len; i++) {
    if ((result + i)->error.len) {
      in3_log_debug("curl: failed for %s\n", urls[i]);
      return IN3_ETRANS; // return error if even one failed
    }
  }
  return IN3_OK;
}

in3_ret_t send_curl(in3_request_t* req) {
  // set the init-time
  in3_ret_t res;
  uint64_t  start = current_ms();
#ifdef CURL_BLOCKING
  res = send_curl_blocking((const char**) req->urls, req->urls_len, req->payload, req->results, req->timeout);
#else
  res = send_curl_nonblocking((const char**) req->urls, req->urls_len, req->payload, req->results, req->timeout);
#endif
  uint32_t t = (uint32_t)(current_ms() - start);
  if (!req->times) req->times = _malloc(sizeof(uint32_t) * req->urls_len);
  for (int i = 0; i < req->urls_len; i++) req->times[i] = t;
  return res;
}

/**
 * registers curl as a default transport.
 */
void in3_register_curl() {
  in3_set_default_transport(send_curl);
}
