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
#include "../../core/client/plugin.h"
#include "../../core/client/version.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include <curl/curl.h>
#include <string.h>

#ifndef CURL_MAX_PARALLEL
#define CURL_MAX_PARALLEL 50
#endif
typedef struct {
  CURLM*             cm;
  uint32_t           start;
  struct curl_slist* headers;

} in3_curl_t;

/*
struct MemoryStruct {
  char *memory = NULL;
  size_t size;
};
 */
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  in3_response_t* r = (in3_response_t*) userp;
  sb_add_range(&r->data, contents, 0, size * nmemb);
  return size * nmemb;
}

static void readDataNonBlocking(CURLM* cm, const char* url, const char* payload, struct curl_slist* headers, in3_response_t* r, uint32_t timeout) {
  CURL*     curl;
  CURLMcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (payload && *payload) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(payload));
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (uint64_t) timeout / 1000L);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, (void*) r);

    /* Perform the request, res will get the return code */
    res = curl_multi_add_handle(cm, curl);
    if (res != CURLM_OK) {
      sb_add_chars(&r->data, "Invalid response:");
      sb_add_chars(&r->data, (char*) curl_easy_strerror((CURLcode) res));
      r->state = IN3_ERPC;
    }
  }
  else {
    sb_add_chars(&r->data, "no curl:");
    r->state = IN3_ECONFIG;
  }
}

in3_ret_t receive_next(in3_request_t* req) {
  in3_curl_t* c = req->cptr;
  CURLMsg*    msg;
  int         msgs_left   = -1;
  int         still_alive = 1;

  do {
    curl_multi_perform(c->cm, &still_alive);

    while ((msg = curl_multi_info_read(c->cm, &msgs_left))) {
      CURL*           e = msg->easy_handle;
      in3_response_t* response;
      long            response_code;
      curl_easy_getinfo(e, CURLINFO_PRIVATE, &response);
      curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &response_code);
      if (msg->msg == CURLMSG_DONE) {
        CURLcode res = msg->data.result;
        if (res != CURLE_OK) {
          sb_add_chars(&response->data, "Invalid response:");
          sb_add_chars(&response->data, (char*) curl_easy_strerror((CURLcode) res));
          response->state = IN3_ERPC;
        }
        else if (response_code > 100 && response_code < 400)
          response->state = IN3_OK;
        else {
          if (!response->data.len)
            sb_add_chars(&response->data, "returned with invalid status code");
          response->state = IN3_ERPC;
        }
        curl_multi_remove_handle(c->cm, e);
        curl_easy_cleanup(e);
        response->time = current_ms() - c->start;
        return response->state;
      }
    }

    if (still_alive)
      curl_multi_wait(c->cm, NULL, 0, 1000, NULL);

  } while (still_alive);
  return msgs_left > 0 ? IN3_EFIND : IN3_ERPC;
}

in3_ret_t cleanup(in3_curl_t* c) {

  curl_slist_free_all(c->headers);
  curl_multi_cleanup(c->cm);
  _free(c);
  return IN3_OK;
}

in3_ret_t send_curl_nonblocking(in3_request_t* req) {

  in3_curl_t* c = _malloc(sizeof(in3_curl_t));
  req->cptr     = c;
  c->cm         = curl_multi_init();
  c->start      = current_ms();
  curl_multi_setopt(c->cm, CURLMOPT_MAXCONNECTS, (long) CURL_MAX_PARALLEL);
  struct curl_slist* headers = curl_slist_append(NULL, "Accept: application/json");
  if (req->payload && *req->payload)
    headers = curl_slist_append(headers, "Content-Type: application/json");
  headers    = curl_slist_append(headers, "charsets: utf-8");
  c->headers = curl_slist_append(headers, "User-Agent: in3 curl " IN3_VERSION);

  // create requests
  for (unsigned int i = 0; i < req->urls_len; i++) readDataNonBlocking(c->cm, req->urls[i], req->payload, c->headers, req->ctx->raw_response + i, req->ctx->client->timeout);
  in3_ret_t res = receive_next(req);
  if (req->urls_len == 1) {
    cleanup(c);
    req->cptr = NULL;
  }
  return res;
}

static void readDataBlocking(const char* url, char* payload, in3_response_t* r, uint32_t timeout) {
  CURL*    curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (payload && *payload) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(payload));
    }
    struct curl_slist* headers = NULL;
    headers                    = curl_slist_append(headers, "Accept: application/json");
    if (payload && *payload)
      headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");
    headers = curl_slist_append(headers, "User-Agent: in3 curl " IN3_VERSION);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (uint64_t) timeout / 1000L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      sb_add_chars(&r->data, "Invalid response:");
      sb_add_chars(&r->data, (char*) curl_easy_strerror(res));
      r->state = IN3_ERPC;
    }
    else
      r->state = IN3_OK;

    curl_slist_free_all(headers);
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  else {
    sb_add_chars(&r->data, "no curl:");
    r->state = IN3_ERPC;
  }
}

in3_ret_t send_curl_blocking(const char** urls, int urls_len, char* payload, in3_response_t* result, uint32_t timeout) {
  int i;
  for (i = 0; i < urls_len; i++)
    readDataBlocking(urls[i], payload, result + i, timeout);
  for (i = 0; i < urls_len; i++) {
    if ((result + i)->state) {
      in3_log_debug("curl: failed for %s\n", urls[i]);
      return IN3_ETRANS; // return error if even one failed
    }
  }
  return IN3_OK;
}

in3_ret_t send_curl(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_request_t* req = plugin_ctx;
  // set the init-time
#ifdef CURL_BLOCKING
  in3_ret_t res;
  uint64_t  start = current_ms();
  res             = send_curl_blocking((const char**) req->urls, req->urls_len, req->payload, req->ctx->raw_response, req->ctx->client->timeout);
  uint32_t t      = (uint32_t)(current_ms() - start);
  for (int i = 0; i < req->urls_len; i++) req->ctx->raw_response[i].time = t;
  return res;
#else
  switch (action) {
    case PLGN_ACT_TRANSPORT_SEND:
      return send_curl_nonblocking(req);
    case PLGN_ACT_TRANSPORT_RECEIVE:
      return receive_next(req);
    case PLGN_ACT_TRANSPORT_CLEAN:
      return cleanup(req->cptr);
    default:
      return IN3_EINVAL;
  }
#endif
}

/**
 * registers curl as a default transport.
 */
in3_ret_t in3_register_curl(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_TRANSPORT, send_curl, NULL, true);
}
