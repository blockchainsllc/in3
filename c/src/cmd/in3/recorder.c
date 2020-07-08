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
  char*              file;
  in3_transport_send transport;
  FILE*              f;
} recorder_out_t;

typedef struct {
  char*  name;
  char** args;
  int    argl;
} recorder_entry_t;

static recorder_out_t out = {
    .file      = NULL,
    .transport = NULL,
    .f         = NULL};

static int rand_out(void* s) {
  UNUSED_VAR(s);
  int r = rand();
  fprintf(out.f, ":: rand %i\n\n", r);
  fflush(out.f);
  return r;
}

recorder_entry_t read_entry(sb_t* sb) {
  recorder_entry_t entry = {0};

  char buffer[1024];
  while (fgets(buffer, 1023, out.f)) {
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
  switch (req->action) {
    case REQ_ACTION_SEND:
      entry = read_entry(&sb);
      if (!entry.name || strcmp(entry.name, "request")) die("expected request in recorder!");
      entry_free(&entry, &sb);

    case REQ_ACTION_RECEIVE:
      entry = read_entry(&sb);
      if (!entry.name || strcmp(entry.name, "response")) die("expected response in recorder!");
      in3_response_t* r = req->ctx->raw_response + atoi(entry.args[0]);
      sb_add_chars(&r->data, sb.data);
      r->state = atoi(entry.args[3]);
      r->time  = atoi(entry.args[4]);

      entry_free(&entry, &sb);
      break;
    default:
      break;
  }
  return 0;
}
static in3_ret_t recorder_transport_out(in3_request_t* req) {
  in3_chain_t*  chain = in3_find_chain(req->ctx->client, req->ctx->client->chain_id);
  node_match_t* m     = req->ctx->nodes;
  in3_ret_t     res   = out.transport(req);
  if (req->action == REQ_ACTION_SEND) {
    fprintf(out.f, ":: request ");
    for (int i = 0; m; i++, m = m->next)
      fprintf(out.f, "%s ", ctx_get_node(chain, m)->url);
    fprintf(out.f, "\n     %s\n\n", req->payload);
    fflush(out.f);
  }
  if (req->action != REQ_ACTION_CLEANUP) {
    m = req->ctx->nodes;
    for (int i = 0; m; i++, m = m->next) {
      in3_response_t* r = req->ctx->raw_response + i;
      if (r->time) {
        fprintf(out.f, ":: response %i %s %s %i %i\n", i, d_get_stringk(req->ctx->requests[0], K_METHOD), ctx_get_node(chain, m)->url, r->state, r->time);
        char* data = format_json(r->data.data ? r->data.data : "");
        fprintf(out.f, "%s\n\n", data);
        fflush(out.f);
        _free(data);
      }
    }
  }
  return res;
}

void recorder_write_start(in3_t* c, char* file) {
  out.file      = file;
  out.transport = c->transport;
  c->transport  = recorder_transport_out;
  out.f         = fopen(file, "w");
  in3_set_func_rand(rand_out);
}

void recorder_read_start(in3_t* c, char* file) {
  out.file      = file;
  out.transport = c->transport;
  c->transport  = recorder_transport_in;
  out.f         = fopen(file, "r");
  in3_set_func_rand(rand_in);
}