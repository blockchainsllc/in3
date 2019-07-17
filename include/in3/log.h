// @PUBLIC_HEADER
/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>

#define LOG_VERSION "0.1.0"

typedef void (*in3_log_LockFn)(void* udata, int lock);

typedef enum { LOG_TRACE,
               LOG_DEBUG,
               LOG_INFO,
               LOG_WARN,
               LOG_ERROR,
               LOG_FATAL } in3_log_lvl_t;

#define in3_log_trace(...) in3_log(LOG_TRACE, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_debug(...) in3_log(LOG_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_info(...) in3_log(LOG_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_warn(...) in3_log(LOG_WARN, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_error(...) in3_log(LOG_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_fatal(...) in3_log(LOG_FATAL, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define in3_log_level_is(lvl) ((lvl) == in3_log_get_level())

/**
 * in3_log_set_*() functions are not thread-safe. 
 * It is expected that these initialization functions will be called from the main thread before 
 * spawning more threads.
 */
void          in3_log_set_udata(void* udata);
void          in3_log_set_lock(in3_log_LockFn fn);
void          in3_log_set_fp(FILE* fp);
void          in3_log_set_level(in3_log_lvl_t level);
in3_log_lvl_t in3_log_get_level();
void          in3_log_set_quiet(int enable);
void          in3_log_set_prefix(const char* prefix);

/* in3_log() function can be made thread-safe using the in3_log_set_lock() function */
void in3_log(in3_log_lvl_t level, const char* file, const char* function, int line, const char* fmt, ...);

#endif
