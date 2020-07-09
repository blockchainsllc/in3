#include "recorder.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include <math.h>
#include <stdio.h>
static void die(char* msg) {
  fprintf(stderr, COLORT_RED "Error: %s" COLORT_RESET "\n", msg);
  exit(EXIT_FAILURE);
}

typedef struct {
  char*                  file;
  in3_transport_send     transport;
  FILE*                  f;
  in3_storage_handler_t* cache;
} recorder_t;

typedef struct {
  char*  name;
  char** args;
  int    argl;

} recorder_entry_t;

static recorder_t rec = {
    .file      = NULL,
    .transport = NULL,
    .f         = NULL,
    .cache     = NULL};

static int rand_out(void* s) {
  UNUSED_VAR(s);
  int r = rand();
  fprintf(rec.f, ":: rand %i\n\n", r);
  fflush(rec.f);
  return r;
}

recorder_entry_t read_entry(sb_t* sb) {
  recorder_entry_t entry = {0};

  char buffer[1024];
  while (fgets(buffer, 1023, rec.f)) {
    int l = strlen(buffer);
    if (buffer[l - 1] == '\n')
      buffer[--l] = 0;
    if (!l) break;
    if (!entry.name) {
      char* ptr  = strtok(buffer + 3, " ");
      entry.name = _strdupn(ptr, -1);
      while ((ptr = strtok(NULL, " "))) {
        entry.args               = entry.argl ? _realloc(entry.args, sizeof(char*) * (entry.argl + 1), sizeof(char*) * entry.argl) : _malloc(sizeof(char*));
        entry.args[entry.argl++] = _strdupn(ptr, -1);
      }
    } else
      sb_add_chars(sb, buffer);
  }

  return entry;
}
static void entry_free(recorder_entry_t* e, sb_t* sb) {
  if (e->name) _free(e->name);
  for (int i = 0; i < e->argl; i++) _free(e->args[i]);
  _free(e->args);
  if (sb && sb->data) _free(sb->data);
  if (sb) *sb = (sb_t){0};
}
static int rand_in(void* s) {
  UNUSED_VAR(s);
  sb_t             sb    = {0};
  recorder_entry_t entry = read_entry(&sb);
  if (!entry.name || strcmp(entry.name, "rand")) die("expected rand in recorder!");
  if (entry.argl != 1) die("expect one arg for random");
  int r = atoi(entry.args[0]);
  entry_free(&entry, &sb);

  return r;
}

static in3_ret_t recorder_transport_in(in3_request_t* req) {
  sb_t             sb    = {0};
  recorder_entry_t entry = {0};
  if (req->action == REQ_ACTION_SEND) {
    entry = read_entry(&sb);
    if (!entry.name || strcmp(entry.name, "request")) die("expected request in recorder!");
    entry_free(&entry, &sb);
    req->cptr = &rec;
  }
  if (req->action != REQ_ACTION_CLEANUP) {
    entry = read_entry(&sb);
    if (!entry.name || strcmp(entry.name, "response")) die("expected response in recorder!");
    in3_response_t* r = req->ctx->raw_response + atoi(entry.args[0]);
    sb_add_chars(&r->data, sb.data);
    r->state = atoi(entry.args[3]);
    r->time  = atoi(entry.args[4]);
    entry_free(&entry, &sb);
  }

  return 0;
}
static in3_ret_t recorder_transport_out(in3_request_t* req) {
  in3_chain_t*  chain = in3_find_chain(req->ctx->client, req->ctx->client->chain_id);
  node_match_t* m     = req->ctx->nodes;
  in3_ret_t     res   = rec.transport(req);
  if (req->action == REQ_ACTION_SEND) {
    fprintf(rec.f, ":: request ");
    for (int i = 0; m; i++, m = m->next)
      fprintf(rec.f, "%s ", ctx_get_node(chain, m)->url);
    fprintf(rec.f, "\n     %s\n\n", req->payload);
    fflush(rec.f);
  }
  if (req->action != REQ_ACTION_CLEANUP) {
    m = req->ctx->nodes;
    for (int i = 0; m; i++, m = m->next) {
      in3_response_t* r = req->ctx->raw_response + i;
      if (r->time) {
        fprintf(rec.f, ":: response %i %s %s %i %i\n", i, d_get_stringk(req->ctx->requests[0], K_METHOD), ctx_get_node(chain, m)->url, r->state, r->time);
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
  sb_t             sb    = {0};
  recorder_entry_t entry = read_entry(&sb);
  if (!entry.name || strcmp(entry.name, "cache")) die("expected cache in recorder!");
  if (entry.argl != 2) die("expect 2 args for cache");
  if (strcmp(key, entry.args[0])) die("wrong cache key");
  bytes_t* found = atoi(entry.args[1]) ? hex_to_new_bytes(sb.data, sb.len) : NULL;
  entry_free(&entry, &sb);

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

void recorder_write_start(in3_t* c, char* file) {
  rec.file      = file;
  rec.transport = c->transport;
  c->transport  = recorder_transport_out;
  rec.f         = fopen(file, "w");
  rec.cache     = c->cache;
  in3_set_func_rand(rand_out);
  in3_set_storage_handler(c, rec_get_item_out, rec_set_item_out, rec_clear_out, &rec);
}

void recorder_read_start(in3_t* c, char* file) {
  rec.file      = file;
  rec.transport = c->transport;
  c->transport  = recorder_transport_in;
  rec.f         = fopen(file, "r");
  in3_set_func_rand(rand_in);
  in3_set_storage_handler(c, rec_get_item_in, rec_set_item_in, rec_clear_in, &rec);
}
