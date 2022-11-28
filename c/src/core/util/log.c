/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "../../signer/pk-signer/rpcs.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __ZEPHYR__
#include <zephyr.h>
#endif
#include "log.h"
#include "mem.h"
#include "stringbuilder.h"
static char next_char(char** p, const char* delim) {
  if (!*p || !**p) return 0;
  while (*(++(*p))) {
    if ((**p == 33 || **p == 0x1b) && *((*p) + 1)) {
      (*p)++;
      continue; // escape for color
    }
    if (strchr(delim, **p)) return **p;
  }
  return 0;
}
static void remove_pk_args(sb_t* sb, char* k) {
  for (char* p = sb->data; (p = (p && p[0]) ? str_find(p + 1, k) : NULL);) {
    p += strlen(k) - 1;
    if (!next_char(&p, "[{\",}]")) return;
    bool is_array = *p == '[';
    if (is_array && next_char(&p, "[{\",}]") != '"') continue;
    while (*p == '"') {
      char* start = p + 1;
      if (next_char(&p, "\"")) {
        while (start < p) *(start++) = 'x';
        if (is_array && next_char(&p, "[{\",}]") == ',' && next_char(&p, "[{\",}]") == '"') continue;
      }
      break;
    }
  }
}

// we remove any log output, which may contain a private key
static void remove_pk(sb_t* sb) {
  remove_pk_args(sb, "\"pk\":");
  remove_pk_args(sb, FN_IN3_ADDRAWKEY);
  remove_pk_args(sb, FN_IN3_ADDMNEMONIC);
  remove_pk_args(sb, FN_IN3_ADDJSONKEY);
}

static struct {
  void*           udata;
  in3_log_LockFn  lock;
  FILE*           fp;
  in3_log_level_t level;
  int             quiet;
  const char*     prefix;
  int             enable_prefix;
} L = {.quiet = 1, .enable_prefix = 1,
#ifdef DEBUG
       .level = LOG_DEBUG
#else
       .level = LOG_INFO
#endif
};

static const char* level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char* level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void lock(void) {
  if (L.lock) {
    L.lock(L.udata, 1);
  }
}

static void unlock(void) {
  if (L.lock) {
    L.lock(L.udata, 0);
  }
}

void in3_log_set_udata_(void* udata) {
  L.udata = udata;
}

void in3_log_set_lock_(in3_log_LockFn fn) {
  L.lock = fn;
}

void in3_log_set_fp_(FILE* fp) {
  L.fp = fp;
}

void in3_log_set_level_(in3_log_level_t level) {
  L.level = level;
}

in3_log_level_t in3_log_get_level_() {
  return L.level;
}

void in3_log_set_quiet_(int enable) {
  L.quiet = enable ? 1 : 0;
}

void in3_log_set_prefix_(const char* prefix) {
  L.prefix = prefix;
}

void in3_log_enable_prefix_() {
  L.enable_prefix = 1;
}

int in3_log_is_prefix_enabled_() {
  return L.enable_prefix;
}

void in3_log_disable_prefix_() {
  L.enable_prefix = 0;
}

void in3_log_(in3_log_level_t level, const char* filename, const char* function, int line, const char* fmt, ...) {
  if (level < L.level) {
    return;
  }
  else if (L.quiet && !L.fp) {
    return;
  }

  const char* file = strrchr(filename, '/');
  file             = file ? file + 1 : filename;

  /* Acquire lock */
  lock();

  /* Log to stderr */
  if (!L.quiet) {
#ifndef __ZEPHYR__
    va_list args;
    char    buf[16];
    _localtime(buf);

    if (L.enable_prefix) {

      if (L.prefix == NULL) {
#ifdef LOG_USE_COLOR
        fprintf(
            stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:%s():\x1b[0m ",
            buf, level_colors[level], level_names[level], file, line, function);
#else
        fprintf(stderr, "%s %-5s %s:%d:%s(): ", buf, level_names[level], file, line, function);
#endif
      }
      else {
        fprintf(stderr, "%s", L.prefix);
      }
    }

    va_start(args, fmt);
    sb_t sb = {0};
    sb_vprintx(&sb, fmt, args);
    va_end(args);
    remove_pk(&sb);
    fprintf(stderr, "%s", sb.data);
    _free(sb.data);
    fflush(stderr);
#else
    va_list args;
    char    buf[16];
    _localtime(buf);
    if (L.prefix == NULL) {
#ifdef LOG_USE_COLOR
      printk(
          "%s %s%-5s\x1b[0m \x1b[90m%s:%d:%s():\x1b[0m ",
          buf, level_colors[level], level_names[level], file, line, function);
#else
      printk("%s %-5s %s:%d:%s(): ", buf, level_names[level], file, line, function);
#endif
    }
    else {
      printk("%s", L.prefix);
    }

    va_start(args, fmt);
    sb_t sb = {0};
    sb_vprintx(&sb, fmt, args);
    va_end(args);
    remove_pk(&sb);
    printk("%s", sb.data);
    fflush(stderr);
    _free(sb.data);
#endif
    // fflush(stderr);
  }

  /* Log to file */
  if (L.fp) {
    va_list args;
    char    buf[32];
    _localtime(buf);
    if (L.enable_prefix) {
      if (L.prefix == NULL)
        fprintf(L.fp, "%s %-5s %s:%s:%d: ", buf, level_names[level], file, function, line);
      else
        fprintf(L.fp, "%s", L.prefix);
    }
    va_start(args, fmt);
    sb_t sb = {0};
    sb_vprintx(&sb, fmt, args);
    va_end(args);
    remove_pk(&sb);
    fprintf(L.fp, "%s", sb.data);
    fflush(L.fp);
    _free(sb.data);
  }

  /* Release lock */
  unlock();
}
