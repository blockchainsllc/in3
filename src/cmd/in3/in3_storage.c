
#include "../../core/client/client.h"
#include "../../core/util/mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

static char* _HOME_DIR = NULL;
static char* get_storage_dir() {
  if (_HOME_DIR == NULL) {
#if defined(_WIN32)
    char* home = getenv("USERPROFILE");
    if (!home) home = ".";
    _HOME_DIR = _malloc(strlen(home) + 8);
    sprintf(_HOME_DIR, "%s\\.in3\\", home);
    _mkdir(_HOME_DIR);
#else
    char* home = getenv("HOME");
    if (!home) home = ".";
    _HOME_DIR = _malloc(strlen(home) + 8);
    sprintf(_HOME_DIR, "%s/.in3/", home);
    mode_t old_umask;
    old_umask = umask(0);
    mkdir(_HOME_DIR, 0777);
    umask(old_umask);
#endif
  }
  return _HOME_DIR;
}

static char* create_path(char* key) {
  char* path = _malloc(strlen(get_storage_dir()) + strlen(key) + 5);
  sprintf(path, "%s%s", get_storage_dir(), key);
  return path;
}

bytes_t* storage_get_item(void* cptr, char* key) {
  UNUSED_VAR(cptr);
  char* path = create_path(key);

  FILE* file = fopen(path, "r");
  if (file) {
    size_t   allocated = 1024;
    size_t   len       = 0;
    uint8_t* buffer    = _malloc(1024);
    size_t   r;

    while (1) {
      r = fread(buffer + len, 1, allocated - len, file);
      len += r;
      if (feof(file)) break;
      buffer = _realloc(buffer, allocated * 2, allocated);
      allocated *= 2;
    }
    fclose(file);

    bytes_t* res = _malloc(sizeof(bytes_t));
    res->data    = buffer;
    res->len     = len;
    return res;
  }
  return NULL;
}

void storage_set_item(void* cptr, char* key, bytes_t* content) {
  UNUSED_VAR(cptr);
  char* path = create_path(key);
  FILE* file = fopen(path, "wb");
  if (file) {
    fwrite(content->data, sizeof(uint8_t), content->len, file);
    fclose(file);
  }
  _free(path);
}
