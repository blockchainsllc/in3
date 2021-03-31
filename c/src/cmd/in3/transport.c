#include "../../core/client/request_internal.h"
#include "helper.h"
#ifdef USE_CURL
#include "../../transport/curl/in3_curl.h"
#elif USE_WINHTTP
#include "../../transport/winhttp/in3_winhttp.h"
#else
#include "../../transport/http/in3_http.h"
#endif

static bool     out_response = false;
static bytes_t* last_response;
static bytes_t  in_response      = {.data = NULL, .len = 0};
static bool     only_show_raw_tx = false;
in3_ret_t       debug_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);

  in3_http_request_t* req = plugin_ctx;
  if (action == PLGN_ACT_TRANSPORT_SEND) {
#ifndef DEBUG
    if (*get_output_conf() & out_debug)
      fprintf(stderr, "send request to %s: \n" COLORT_RYELLOW "%s" COLORT_RESET "\n", req->urls_len ? req->urls[0] : "none", req->payload);
#endif
    if (in_response.len) {
      for (unsigned int i = 0; i < req->urls_len; i++) {
        req->req->raw_response[i].state = IN3_OK;
        sb_add_range(&req->req->raw_response[i].data, (char*) in_response.data, 0, in_response.len);
        req->req->raw_response[i].state = IN3_OK;
      }
      return 0;
    }
    if (only_show_raw_tx && str_find(req->payload, "\"method\":\"eth_sendRawTransaction\"")) {
      char* data         = str_find(req->payload, "0x");
      *strchr(data, '"') = 0;
      recorder_print(0, "%s\n", data);
      recorder_exit(EXIT_SUCCESS);
    }
  }
#ifdef USE_CURL
  in3_ret_t r = send_curl(NULL, action, plugin_ctx);
#elif USE_WINHTTP
  in3_ret_t r = send_winhttp(NULL, action, plugin_ctx);
#elif TRANSPORTS
  in3_ret_t r = send_http(NULL, action, plugin_ctx);
#else
  in3_ret_t r = req_set_error(req->req, "No transport supported in the client", IN3_ECONFIG);
#endif
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    last_response = b_new((uint8_t*) req->req->raw_response[0].data.data, req->req->raw_response[0].data.len);
#ifndef DEBUG
    if (*get_output_conf() & out_debug) {
      if (req->req->raw_response[0].state == IN3_OK)
        fprintf(stderr, "success response \n" COLORT_RGREEN "%s" COLORT_RESET "\n", req->req->raw_response[0].data.data);
      else
        fprintf(stderr, "error response \n" COLORT_RRED "%s" COLORT_RESET "\n", req->req->raw_response[0].data.data);
    }
#endif
  }
  return r;
}
static char*     test_name = NULL;
static in3_ret_t test_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_http_request_t* req = plugin_ctx;
#ifdef USE_CURL
  in3_ret_t r = send_curl(NULL, action, plugin_ctx);
#elif USE_WINHTTP
  in3_ret_t r = send_winhttp(NULL, action, plugin_ctx);
#elif TRANSPORTS
  in3_ret_t r = send_http(NULL, action, plugin_ctx);
#else
  in3_ret_t r = action && plugin_ctx != NULL ? IN3_OK : IN3_ECONFIG;
#endif
  if (r == IN3_OK) {
    req->payload[strlen(req->payload) - 1] = 0;
    recorder_print(0, "[{ \"descr\": \"%s\",\"chainId\": \"0x1\", \"verification\": \"proof\",\"binaryFormat\": false, \"request\": %s, \"response\": %s }]", test_name, req->payload + 1, req->req->raw_response->data.data);
    recorder_exit(0);
  }

  return r;
}

void init_transport(in3_t* c) {
  in3_plugin_register(c, PLGN_ACT_TRANSPORT, debug_transport, NULL, true);
#ifdef USE_WINHTTP
  configure(c, "requestCount", "1");
#else
  configure(c, "requestCount", "2");
#endif
}

bool set_test_transport(in3_t* c, char* name) {
  test_name = name;
  in3_plugin_register(c, PLGN_ACT_TRANSPORT, test_transport, NULL, true);
  return true;
}

bool set_onlyshow_rawtx() {
  only_show_raw_tx = true;
  return true;
}

bool is_onlyshow_rawtx() {
  return only_show_raw_tx;
}

bool set_response_file(bool is_in) {
  if (is_in)
    in_response = readFile(stdin);
  else
    out_response = true;
  return true;
}

void check_last_output() {
  if (out_response && last_response) {
    char* r = alloca(last_response->len + 1);
    memcpy(r, last_response->data, last_response->len);
    r[last_response->len] = 0;
    recorder_print(0, "%s\n", r);
    recorder_exit(0);
  }
}