#include "recorder.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include <math.h>
#include <stdio.h>

typedef struct recorder_entry {
  char*                  name;
  char**                 args;
  int                    argl;
  sb_t                   content;
  struct recorder_entry* next;

} recorder_entry_t;

typedef struct {
  char*                  file;
  in3_plugin_act_fn      transport;
  FILE*                  f;
  in3_storage_handler_t* cache;
  uint64_t               time;
  recorder_entry_t*      queue;
} recorder_t;

static recorder_t rec = {
    .file      = NULL,
    .transport = NULL,
    .f         = NULL,
    .cache     = NULL,
    .queue     = NULL,
    .time      = 0};

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
    if (!l) break;
    if (!entry) {
      entry       = _calloc(sizeof(recorder_entry_t), 1);
      char* ptr   = strtok(buffer + 3, " ");
      entry->name = _strdupn(ptr, -1);
      while ((ptr = strtok(NULL, " "))) {
        entry->args                = entry->argl ? _realloc(entry->args, sizeof(char*) * (entry->argl + 1), sizeof(char*) * entry->argl) : _malloc(sizeof(char*));
        entry->args[entry->argl++] = _strdupn(ptr, -1);
      }
    } else
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
  in3_request_t* req = plugin_ctx;
  if (action == PLGN_ACT_TRANSPORT_SEND) {
    entry_free(next_entry("request", NULL));
    req->cptr = &rec;
  }
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    recorder_entry_t* entry = next_entry("response", d_get_stringk(req->ctx->requests[0], K_METHOD));
    in3_response_t*   r     = req->ctx->raw_response + atoi(entry->args[1]);
    sb_add_chars(&r->data, entry->content.data);
    r->state = atoi(entry->args[3]);
    r->time  = atoi(entry->args[4]);
    entry_free(entry);
  }

  return 0;
}

static in3_ret_t recorder_transport_out(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_request_t* req   = plugin_ctx;
  in3_chain_t*   chain = in3_find_chain(req->ctx->client, req->ctx->client->chain_id);
  node_match_t*  m     = req->ctx->nodes;
  in3_ret_t      res   = rec.transport(NULL, action, plugin_ctx);
  if (action == PLGN_ACT_TRANSPORT_SEND) {
    fprintf(rec.f, ":: request ");
    for (int i = 0; m; i++, m = m->next)
      fprintf(rec.f, "%s ", ctx_get_node(chain, m)->url);
    fprintf(rec.f, "\n     %s\n\n", req->payload);
    fflush(rec.f);
  }
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    m = req->ctx->nodes;
    for (int i = 0; m; i++, m = m->next) {
      in3_response_t* r = req->ctx->raw_response + i;
      if (r->time) {
        fprintf(rec.f, ":: response %s %i %s %i %i\n", d_get_stringk(req->ctx->requests[0], K_METHOD), i, ctx_get_node(chain, m)->url, r->state, r->time);
        char* data = format_json(r->data.data ? r->data.data : "");
        fprintf(rec.f, "%s\n\n", data);
        fflush(rec.f);
        _free(data);
      }
    }
  }
  return res;
}

bytes_t* rec_get_item_in(void* cptr, const char* key) {
  UNUSED_VAR(cptr);
  UNUSED_VAR(key);
  recorder_entry_t* entry = next_entry("cache", key);
  bytes_t*          found = atoi(entry->args[1]) ? hex_to_new_bytes(entry->content.data, entry->content.len) : NULL;
  entry_free(entry);

  return found;
}

void rec_set_item_in(void* cptr, const char* key, bytes_t* content) {
  UNUSED_VAR(cptr);
  UNUSED_VAR(key);
  UNUSED_VAR(content);
}

void rec_clear_in(void* cptr) {
  UNUSED_VAR(cptr);
}

bytes_t* rec_get_item_out(void* cptr, const char* key) {
  if (!rec.cache) return NULL;
  bytes_t* found = rec.cache->get_item(cptr, key);
  fprintf(rec.f, ":: cache %s %i\n", key, found ? 1 : 0);
  if (found) {
    char* hex = alloca(found->len * 2 + 1);
    bytes_to_hex(found->data, found->len, hex);
    fprintf(rec.f, "%s\n\n", hex);
  } else
    fprintf(rec.f, "\n");

  return found;
}

void rec_set_item_out(void* cptr, const char* key, bytes_t* content) {
  if (rec.cache) rec.cache->set_item(cptr, key, content);
}

void rec_clear_out(void* cptr) {
  if (rec.cache) rec.cache->clear(cptr);
}
uint64_t static_time(void* t) {
  UNUSED_VAR(t);
  return rec.time;
}

static in3_plugin_t* get_transport(in3_t* c) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_TRANSPORT_SEND) return p;
  }
  return NULL;
}

void recorder_write_start(in3_t* c, char* file, int argc, char* argv[]) {
  in3_plugin_t* p = get_transport(c);
  rec.file        = file;
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "w");
  rec.cache       = c->cache;
  if (p) p->action_fn = recorder_transport_out;
  in3_set_func_rand(rand_out);
  fprintf(rec.f, ":: cmd");
  for (int i = 0; i < argc; i++) fprintf(rec.f, " %s", strcmp(argv[i], "-fo") ? argv[i] : "-fi");
  fprintf(rec.f, "\n\n");
  in3_set_storage_handler(c, rec_get_item_out, rec_set_item_out, rec_clear_out, &rec);
  fprintf(rec.f, ":: time %u\n\n", (uint32_t) in3_time(NULL));
}

void recorder_read_start(in3_t* c, char* file) {
  in3_plugin_t* p = get_transport(c);
  rec.file        = file;
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "r");
  if (p) p->action_fn = recorder_transport_in;
  in3_set_func_rand(rand_in);
  in3_set_storage_handler(c, rec_get_item_in, rec_set_item_in, rec_clear_in, &rec);
  recorder_entry_t* entry = next_entry("time", NULL);
  rec.time                = entry->argl >= 1 ? atoll(entry->args[0]) : 0;
  entry_free(entry);
  in3_set_func_time(static_time);
}

void recorder_update_cmd(char* file, int* argc, char** argv[]) {
  rec.file                = file;
  rec.f                   = fopen(file, "r");
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