#include "recorder.h"
#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>

typedef struct recorder_entry {
  char*                  name;
  char**                 args;
  int                    argl;
  sb_t                   content;
  struct recorder_entry* next;

} recorder_entry_t;

typedef struct ptr {
  void*       ptr;
  struct ptr* next;
} ptr_t;

static ptr_t* global_ptr = NULL;
void*         add_global(void* ptr) {
  ptr_t* p   = _malloc(sizeof(ptr_t));
  p->next    = global_ptr;
  p->ptr     = ptr;
  global_ptr = p;
  return ptr;
}

typedef struct {
  in3_plugin_act_fn transport;
  FILE*             f;
  in3_plugin_act_fn cache;
  uint64_t          time;
  recorder_entry_t* queue;
  bool              has_result;
  bool              is_out;
} recorder_t;

static sb_t*      result = NULL;
static recorder_t rec    = {
       .transport  = NULL,
       .f          = NULL,
       .cache      = NULL,
       .queue      = NULL,
       .has_result = false,
       .is_out     = false,
       .time       = 0};

static int rand_out(void* s) {
  UNUSED_VAR(s);
  int r = rand();
  fprintf(rec.f, ":: rand %i\n\n", r);
  fflush(rec.f);
  return r;
}

static inline bool match(recorder_entry_t* entry, const char* type, const char* first_arg) {
  return entry && entry->name && strcmp(type, entry->name) == 0 && (first_arg == NULL || (entry->argl && strcmp(entry->args[0], first_arg) == 0));
}

static recorder_entry_t* read_one_entry() {
  recorder_entry_t* entry = NULL;
  char              buffer[1024];
  while (fgets(buffer, 1023, rec.f)) {
    int l = strlen(buffer);
    if (buffer[l - 1] == '\n')
      buffer[--l] = 0;
    if (!l) {
      if (entry)
        break;
      else
        continue;
    }
    if (!entry) {
      entry       = add_global(_calloc(sizeof(recorder_entry_t), 1));
      char* ptr   = strtok(buffer + 3, " ");
      entry->name = _strdupn(ptr, -1);
      while ((ptr = strtok(NULL, " "))) {
        entry->args                = entry->argl ? _realloc(entry->args, sizeof(char*) * (entry->argl + 1), sizeof(char*) * entry->argl) : _malloc(sizeof(char*));
        entry->args[entry->argl++] = add_global(_strdupn(ptr, -1));
      }
    }
    else
      sb_add_chars(&entry->content, buffer);
  }
  return entry;
}

recorder_entry_t* next_entry(const char* type, const char* firs_arg) {

  recorder_entry_t *last = rec.queue, *end = NULL;
  for (recorder_entry_t* n = last; n; last = n, n = n->next) {
    if (match((end = n), type, firs_arg)) {
      if (last == n)
        rec.queue = n->next;
      else
        last->next = n->next;
      return n;
    }
  }

  do {
    if (match((last = read_one_entry()), type, firs_arg))
      return last;
    if (!last) {
      fprintf(stderr, COLORT_RED "Error: expected entry %s %s but did not find it!" COLORT_RESET "\n", type, firs_arg ? firs_arg : "");
      exit(EXIT_FAILURE);
    }
    if (end)
      end->next = last;
    else
      rec.queue = last;
    end = last;

  } while (true);
}

static void entry_free(recorder_entry_t* e) {
  if (e->name) _free(e->name);
  for (int i = 0; i < e->argl; i++) _free(e->args[i]);
  _free(e->args);
  if (e->content.data) _free(e->content.data);
  _free(e);
}
static int rand_in(void* s) {
  UNUSED_VAR(s);
  recorder_entry_t* entry = next_entry("rand", NULL);
  int               r     = atoi(entry->args[0]);
  entry_free(entry);
  return r;
}

static in3_ret_t recorder_transport_in(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_http_request_t* req = plugin_ctx;
  if (action == PLGN_ACT_TRANSPORT_SEND) {
    entry_free(next_entry("request", NULL));
    req->cptr = &rec;
  }
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    recorder_entry_t* entry = next_entry("response", d_get_string(req_get_request(req->req, 0), K_METHOD));
    in3_response_t*   r     = req->req->raw_response + atoi(entry->args[1]);
    sb_add_chars(&r->data, entry->content.data);
    r->state = atoi(entry->args[3]);
    r->time  = atoi(entry->args[4]);
    entry_free(entry);
  }

  return 0;
}

static in3_ret_t recorder_transport_out(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_http_request_t* req = plugin_ctx;
  node_match_t*       m   = req->req->in3_state ? req->req->in3_state->nodes : NULL;
  in3_ret_t           res = rec.transport(NULL, action, plugin_ctx);
  if (action == PLGN_ACT_TRANSPORT_SEND) {
    fprintf(rec.f, ":: request ");
    char* rpc = d_get_string(d_get(req_get_request(req->req, 0), K_IN3), K_RPC);
    if (rpc)
      fprintf(rec.f, "%s ", rpc);
    else {
      for (int i = 0; m; i++, m = m->next)
        fprintf(rec.f, "%s ", m->url);
    }
    fprintf(rec.f, "\n     %s\n\n", req->payload);
    fflush(rec.f);
  }
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    m         = req->req->in3_state ? req->req->in3_state->nodes : NULL;
    char* rpc = d_get_string(d_get(req_get_request(req->req, 0), K_IN3), K_RPC);
    int   l   = rpc ? 1 : req_nodes_len(m);
    for (int i = 0; i < l; i++, m = m ? m->next : NULL) {
      in3_response_t* r = req->req->raw_response + i;
      if (m) rpc = m->url;
      if (r->time) {
        fprintf(rec.f, ":: response %s %i %s %i %i\n", d_get_string(req_get_request(req->req, 0), K_METHOD), i, rpc, r->state, r->time);
        char* data = format_json(r->data.data ? r->data.data : "");
        fprintf(rec.f, "%s\n\n", data);
        fflush(rec.f);
        _free(data);
      }
    }
  }
  return res;
}

in3_ret_t storage_in(void* data, in3_plugin_act_t action, void* arg) {
  UNUSED_VAR(data);
  switch (action) {
    case PLGN_ACT_CACHE_CLEAR:
    case PLGN_ACT_CACHE_SET:
      return IN3_OK;
    case PLGN_ACT_CACHE_GET: {
      in3_cache_ctx_t*  ctx   = arg;
      recorder_entry_t* entry = next_entry("cache", ctx->key);
      ctx->content            = atoi(entry->args[1]) ? hex_to_new_bytes(entry->content.data, entry->content.len) : NULL;
      entry_free(entry);
      return ctx->content ? IN3_OK : IN3_EIGNORE;
    }
    default:
      return IN3_EINVAL;
  }
}

in3_ret_t storage_out(void* data, in3_plugin_act_t action, void* arg) {
  in3_ret_t ret = rec.cache ? rec.cache(data, action, arg) : IN3_EIGNORE;
  if (rec.cache && action == PLGN_ACT_CACHE_GET) {
    in3_cache_ctx_t* ctx   = arg;
    bytes_t*         found = ctx->content;
    fprintf(rec.f, ":: cache %s %i\n", ctx->key, found ? 1 : 0);
    if (found) {
      char* hex = alloca(found->len * 2 + 1);
      bytes_to_hex(found->data, found->len, hex);
      fprintf(rec.f, "%s\n\n", hex);
    }
    else
      fprintf(rec.f, "\n");
  }
  return ret;
}

uint64_t static_time(void* t) {
  UNUSED_VAR(t);
  return rec.time;
}

static in3_plugin_t* get_plugin(in3_t* c, in3_plugin_act_t action) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & action) return p;
  }
  return NULL;
}

static void recorder_init(in3_t* c, char* file) {
  in3_plugin_t* p = get_plugin(c, PLGN_ACT_TRANSPORT_SEND);
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "w");
  if (p) p->action_fn = recorder_transport_out;
  p = get_plugin(c, PLGN_ACT_CACHE_GET);
  if (p) {
    rec.cache    = p->action_fn;
    p->action_fn = storage_out;
  }
  in3_set_func_rand(rand_out);
  fprintf(rec.f, ":: time %u\n\n", (uint32_t) in3_time(NULL));
}

void in3_record(in3_t* c, char* file, bool in) {
  char file_record[32];
  sprintf(file_record, "%s_%s.txt", file, IN3_VERSION);
  if (!in) {
    recorder_init(c, file_record);
  }
  else {
    recorder_read_start(c, file_record);
  }
}

void recorder_write_start(in3_t* c, char* file, int argc, char* argv[]) {
  in3_plugin_t* p = get_plugin(c, PLGN_ACT_TRANSPORT_SEND);
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "w");
  rec.is_out      = true;
  if (p) p->action_fn = recorder_transport_out;
  p = get_plugin(c, PLGN_ACT_CACHE_GET);
  if (p) {
    rec.cache    = p->action_fn;
    p->action_fn = storage_out;
  }
  in3_set_func_rand(rand_out);
  fprintf(rec.f, ":: cmd");
  for (int i = 0; i < argc; i++)
    fprintf(rec.f, " %s", strcmp(argv[i], "-fo") ? argv[i] : "-fi");
  fprintf(rec.f, "\n\n");
  fprintf(rec.f, ":: time %u\n\n", (uint32_t) in3_time(NULL));
}

void recorder_read_start(in3_t* c, char* file) {
  in3_plugin_t* p = get_plugin(c, PLGN_ACT_TRANSPORT_SEND);
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "r");
  if (p) p->action_fn = recorder_transport_in;
  p = get_plugin(c, PLGN_ACT_CACHE_GET);
  if (p)
    p->action_fn = storage_in;
  in3_set_func_rand(rand_in);
  recorder_entry_t* entry = next_entry("time", NULL);
  rec.time                = entry->argl >= 1 ? atoll(entry->args[0]) : 0;
  entry_free(entry);
  in3_set_func_time(static_time);
}

void recorder_update_cmd(char* file, int* argc, char** argv[]) {
  rec.f = fopen(file, "r");
  if (!rec.f) {
    fprintf(stderr, "Cannot open recordfile %s : %s\n", file, strerror((int) errno));
    exit(EXIT_FAILURE);
  }
  recorder_entry_t* entry = next_entry("cmd", NULL);
  *argc                   = entry->argl;
  *argv                   = entry->args;
  for (int i = 0; i < entry->argl; i++) {
    if (strcmp(entry->args[i], "-fi") == 0) entry->args[i + 1] = file;
  }
  fclose(rec.f);
  rec.f     = NULL;
  rec.queue = NULL;
}

void recorder_print(int err, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(err ? stderr : stdout, fmt, args);
  va_end(args);

  if (rec.f && rec.is_out) {
    if (!rec.has_result) {
      fprintf(rec.f, ":: result\n");
      rec.has_result = true;
    }
    va_start(args, fmt);
    vfprintf(rec.f, fmt, args);
    va_end(args);
    fflush(rec.f);
  }
  else if (rec.f) {
    if (result == NULL) result = _calloc(1, sizeof(sb_t));
    va_start(args, fmt);
    sb_vprint(result, fmt, args);
    va_end(args);
  }
}
static void remove_whitespace(char* c) {
  if (!c) return;
  char* cp = c;
  for (; *c; c++) {
    if (*c <= 32) continue;
    *cp = *c;
    cp++;
  }
  *cp = 0;
  return;
}
void recorder_exit(int code) {
  if (rec.f && !rec.is_out) {
    recorder_entry_t* entry = next_entry("result", NULL);
    code                    = EXIT_FAILURE;
    remove_whitespace(entry->content.data);
    remove_whitespace(result ? result->data : NULL);
    if (entry->content.len && result == NULL)
      fprintf(stderr, "No result resturned, but expected : %s\n", entry->content.data);
    else if (!entry->content.len && (result != NULL && result->len))
      fprintf(stderr, "No result expected, but got : %s\n", result->data);
    else if (entry->content.len && result && strcmp(result->data, entry->content.data))
      fprintf(stderr, "wrong result!\nexpected: %s\ngot     : %s\n", entry->content.data, result->data);
    else
      code = EXIT_SUCCESS;
  }

  exit(code);
}

void recorder_request(char* req) { UNUSED_VAR(req); }
void recorder_response(char* req) { UNUSED_VAR(req); }
void recorder_error(char* req) { UNUSED_VAR(req); }
void recorder_configure(char* conf) { UNUSED_VAR(conf); }
