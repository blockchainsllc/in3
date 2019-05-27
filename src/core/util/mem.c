#include "mem.h"
#include "debug.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _exit_oom() {
#ifdef EXIT_OOM
  exit(EXIT_OOM);
#else
  exit(EXIT_FAILURE);
#endif
}

#ifdef __ZEPHYR__
void* k_realloc(void* ptr, size_t size, size_t oldsize) {
  void* new = NULL;

  new = k_malloc(size);
  if (!new)
    goto error;

  if (ptr && oldsize) {
    memcpy(new, ptr, oldsize);
    k_free(ptr);
  }

  return new;

error:
  return NULL;
}
#endif /* __ZEPHYR__ */

void* _malloc_(size_t size, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  void* ptr = k_malloc(size);
#else
  void* ptr = malloc(size);
#endif
  if (size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!");
    _exit_oom();
  }
  return ptr;
}

void* _calloc_(size_t n, size_t size, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  void* ptr = k_calloc(n, size);
#else
  void* ptr = calloc(n, size);
#endif
  if (n && size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!");
    _exit_oom();
  }
  return ptr;
}

void* _realloc_(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  ptr = k_realloc(ptr, size, oldsize);
#else
  UNUSED_VAR(oldsize);
  ptr = realloc(ptr, size);
#endif
  if (size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!");
    _exit_oom();
  }
  return ptr;
}

void _free_(void* ptr) {
#ifdef __ZEPHYR__
  k_free(ptr);
#else
  free(ptr);
#endif
}

#ifdef TEST

typedef struct mem_p {
  struct mem_p* next;
  void*         ptr;
  size_t        size;
  int           ct;

} mem_p_t;

static mem_p_t* mem_tracker = NULL;
static int      mem_count   = 0;
static size_t   c_mem       = 0;
static size_t   max_mem     = 0;
static size_t   max_cnt     = 0;
static int      track_count = -1;

void* t_malloc(size_t size, char* file, const char* func, int line) {
  void*    ptr = _malloc_(size, file, func, line);
  mem_p_t* t   = _malloc_(sizeof(mem_p_t), file, func, line);
  t->next      = mem_tracker;
  t->ptr       = ptr;
  t->size      = size;
  t->ct        = ++mem_count;
  t->next      = mem_tracker;
  if (track_count == mem_count)
    printf("Found allocated memory ( %zu bytes ) in %s : %s : %i\n", size, file, func, line);
  mem_tracker = t;
  c_mem += size;
  if (max_mem < c_mem) {
    max_mem = c_mem;
    //    printf("new max allocated memory %zu bytes ( + %zu bytes ) in %s : %s : %i\n", c_mem, size, file, func, line);
    max_cnt = mem_count;
  }
  return ptr;
}

void* t_calloc(size_t n, size_t size, char* file, const char* func, int line) {
  void* ptr = t_malloc(n * size, file, func, line);
  memset(ptr, 0, n * size);
  return ptr;
}

int mem_stack_size() {
  int      n = 0;
  mem_p_t* t = mem_tracker;
  while (t) {
    n++;
    t = t->next;
  }
  return n;
}

void memstack() {
  printf("\n M:");
  mem_p_t* t = mem_tracker;
  while (t) {
    printf("[%p %zu ] ", t->ptr, t->size);
    t = t->next;
  }
}

void t_free(void* ptr, char* file, const char* func, int line) {
  if (ptr == NULL)
    printf("trying to free a null-pointer in %s : %s : %i\n", file, func, line);

  mem_p_t *t = mem_tracker, *prev = NULL;
  while (t) {
    if (ptr == t->ptr) {
      c_mem -= t->size;
      if (max_mem < c_mem) max_mem = c_mem;
      _free_(ptr);
      if (prev == NULL)
        mem_tracker = t->next;
      else
        prev->next = t->next;

      _free_(t);
      return;
    }
    prev = t;
    t    = t->next;
  }

  printf("freeing a pointer which was not allocated anymore %s : %s : %i\n", file, func, line);
  _free_(ptr);
}
void* t_realloc(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line) {
  if (ptr == NULL)
    printf("trying to free a null-pointer in %s : %s : %i\n", file, func, line);

  mem_p_t* t = mem_tracker;
  while (t) {
    if (ptr == t->ptr) {
      c_mem += size - t->size;
      if (max_mem < c_mem) {
        max_mem = c_mem;
        //        printf("            .... realloc %zu                        %s : %s : %i\n", c_mem, file, func, line);
      }
      t->ptr  = _realloc_(ptr, size, oldsize, file, func, line);
      t->size = size;
      return t->ptr;
    }
    t = t->next;
  }
  printf("realloc a pointer which was not allocated anymore %s : %s : %i\n", file, func, line);
  return _realloc_(ptr, size, oldsize, file, func, line);
}

size_t mem_get_max_heap() {
  return max_mem;
}

int mem_get_memleak_cnt() {
  if (mem_tracker != NULL)
    return mem_tracker->ct;
  return 0;
}

void mem_reset(int cnt) {
  track_count = cnt;
  max_mem     = 0;
  c_mem       = 0;
  mem_count   = 0;
  /*
	mem_p_t* t = mem_tracker,*n;
	while (t) {
		_free_(t->ptr);
		n=t;
		t=t->next;
		_free_(n);
	}
    */
  mem_tracker = NULL;
}
#endif /* TEST */
