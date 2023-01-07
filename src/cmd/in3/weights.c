#include "weights.h"
#include "../../nodeselect/full/nodelist.h"
#include "../../nodeselect/full/nodeselect_def.h"
#include "helper.h"
#ifdef USE_CURL
#include "../../transport/curl/in3_curl.h"
#elif USE_WINHTTP
#include "../../transport/winhttp/in3_winhttp.h"
#else
#include "../../transport/http/in3_http.h"
#endif
static uint32_t weightdata = 0;

uint32_t* get_weightsdata() {
  return &weightdata;
}

bool exec_weights(in3_t* c) {
  int run_test_request = weightdata ? ((weightdata & weight_health) ? 2 : 1) : 0;
  c->max_attempts      = 1;
  uint32_t block = 0, b = 0;
  BIT_CLEAR(c->flags, FLAGS_AUTO_UPDATE_LIST);
  uint64_t now  = in3_time(NULL);
  char*    more = "WEIGHT";
  in3_plugin_execute_all(c, PLGN_ACT_CHAIN_CHANGE, c);
  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  if (!nl) return false;
  if (run_test_request == 1) more = "WEIGHT : LAST_BLOCK";
  if (run_test_request == 2) more = "WEIGHT : NAME                   VERSION : RUNNING : HEALTH : LAST_BLOCK";
  recorder_print(0, "   : %-45s : %7s : %5s : %5s: %s\n------------------------------------------------------------------------------------------------\n", "URL", "BL", "CNT", "AVG", more);
  for (unsigned int i = 0; i < nl->nodelist_length; i++) {
    in3_req_t* ctx      = NULL;
    char*      health_s = NULL;
    if (run_test_request) {
      char req[301];
      snprintx(req, 300, "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"in3\":{\"dataNodes\":[\"%A\"]}}", (nl->nodelist + i)->address);
      ctx = req_new(c, req);
      if (ctx) in3_send_req(ctx);
      if (run_test_request == 2) {
        int         health     = 1;
        char*       version    = "";
        char*       node_name  = "";
        uint32_t    running    = 0;
        json_ctx_t* health_res = NULL;
        char        health_url[501];
        char*       urls[1];
        urls[0] = health_url;
        snprintx(health_url, 500, "%s/health", nl->nodelist[i].url);
        in3_http_request_t r    = {0};
        in3_req_t          ctx  = {0};
        ctx.raw_response        = _calloc(sizeof(in3_response_t), 1);
        ctx.raw_response->state = IN3_WAITING;
        ctx.client              = c;
        r.req                   = &ctx;
        r.urls                  = urls;
        r.urls_len              = 1;
        r.payload               = "";
#ifdef USE_CURL
        send_curl(NULL, PLGN_ACT_TRANSPORT_SEND, &r);
#elif USE_WINHTTP
        send_winhttp(NULL, PLGN_ACT_TRANSPORT_SEND, &r);
#elif TRANSPORTS
        send_http(NULL, PLGN_ACT_TRANSPORT_SEND, &r);
#endif
        if (ctx.raw_response->state)
          health = 0;
        else {
          health_res = parse_json(ctx.raw_response->data.data);
          if (!health_res)
            health = 0;
          else {
            node_name    = d_get_string(health_res->result, key("name"));
            version      = d_get_string(health_res->result, key("version"));
            running      = d_get_int(health_res->result, key("running"));
            char* status = d_get_string(health_res->result, key("status"));
            if (!status || strcmp(status, "healthy")) health = 0;
          }
        }
        if (version) {
          char* l = strrchr(version, ':');
          if (l) version = l + 1;
        }
        health_s = sprintx("%-22s %-7s   %7d   %-9s ", node_name ? node_name : "-", version ? version : "-", running, health ? "OK" : "unhealthy");

        if (ctx.raw_response->data.data)
          _free(ctx.raw_response->data.data);
        _free(ctx.raw_response);
        if (health_res) json_free(health_res);
      }
    }
    in3_node_t*        node        = nl->nodelist + i;
    in3_node_weight_t* weight      = nl->weights + i;
    uint64_t           blacklisted = weight->blacklisted_until > now ? weight->blacklisted_until : 0;
    uint32_t           calc_weight = in3_node_calculate_weight(weight, node->capacity, now);
    char *             tr = NULL, *warning = NULL;
    if (ctx) {
      d_token_t* resp = req_get_response(ctx, 0);
      tr              = _malloc(1000);
      if (!ctx->error && d_type(d_get(resp, K_ERROR)) != T_NULL) {
        d_token_t* msg = d_get(resp, K_ERROR);
        if (d_type(msg) == T_OBJECT) msg = d_get(msg, K_MESSAGE);
        snprintx((warning = tr), 999, "%s", msg ? d_string(msg) : "Error-Response!");
      }
      else if (!ctx->error) {
        b = d_get_int(resp, K_RESULT);
        if (block < b) block = b;

        if (b < block - 1)
          snprintx((warning = tr), 999, "#%i ( out of sync : %i blocks behind latest )", b, block - b);
        else if (strncmp(node->url, "https://", 8))
          snprintx((warning = tr), 999, "#%i (missing https, which is required in a browser )", b);
        else if (!IS_APPROX(d_get_int(resp, K_RESULT), d_get_int(d_get(resp, K_IN3), K_CURRENT_BLOCK), 1))
          snprintx((warning = tr), 999, "#%i ( current block mismatch: %i blocks apart )", b,
                   d_get_int(resp, K_RESULT) - d_get_int(d_get(resp, K_IN3), K_CURRENT_BLOCK));
        else
          snprintx(tr, 999, "#%i", b);
      }
      else if (!strlen(node->url) || !node->props) // NOSONAR node->url is a safe nullterminated string
        snprintx((warning = tr), 999, "No URL spcified anymore props = %i ", (int) (node->props & 0xFFFFFF));
      else if ((node->props & NODE_PROP_DATA) == 0)
        snprintx((warning = tr), 999, "The node is marked as not supporting Data-Providing");
      else if (c->proof != PROOF_NONE && (node->props & NODE_PROP_PROOF) == 0)
        snprintx((warning = tr), 999, "The node is marked as able to provide proof");
      else if ((c->flags & FLAGS_HTTP) && (node->props & NODE_PROP_HTTP) == 0)
        snprintx((warning = tr), 999, "The node is marked as able to support http-requests");
      else
        tr = ctx->error;
      if (strnlen(tr, 1000) > 100) tr[100] = 0; // NOSONAR tr is a safe nullterminated string
    }
    if (blacklisted)
      recorder_print(0, COLORT_RED);
    else if (warning)
      recorder_print(0, COLORT_YELLOW);
    else if (!weight->response_count)
      recorder_print(0, COLORT_DARKGRAY);
    else
      recorder_print(0, COLORT_GREEN);
    recorder_print(0, "%2i   %-45s   %7i   %5i   %5i   %5i   %s%s", i, node->url, (int) (blacklisted ? blacklisted - now : 0), weight->response_count, weight->response_count ? (weight->total_response_time / weight->response_count) : 0, calc_weight, health_s ? health_s : "", tr ? tr : "");
    recorder_print(0, COLORT_RESET "\n");
    if (tr && tr != ctx->error) _free(tr);
    if (health_s) _free(health_s);
    if (ctx) req_free(ctx);
  }
  return true;
}