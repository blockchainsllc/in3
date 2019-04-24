#ifndef TEST
#define TEST
#endif

#include <core/util/data.h>
#include <core/util/mem.h>
#include <stdio.h>
#include <string.h>

#include "test_utils.h"

static void test_key_hash_collisions();

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TEST(test_key_hash_collisions);
  return TESTS_END();
}

static char* substr(char* s, char* delim1, char* delim2) {
  char *target, *start, *end;
  target = start = end = NULL;

  if ((start = strstr(s, delim1))) {
    start += strlen(delim1);
    if ((end = strstr(start, delim2))) {
      target = start;
      *end   = '\0';
    }
  }
  return target;
}

static int compare(const void* a, const void* b) {
  return (*(uint16_t*) a - *(uint16_t*) b);
}

static uint16_t collision(uint16_t* hashes, size_t sz) {
  qsort(hashes, sz, sizeof(uint16_t), compare);

  bool dup = false;
  int  i   = 1;
  for (; i < sz; ++i) {
    if (hashes[i] && hashes[i] == hashes[i - 1]) {
      dup = true;
      break;
    }
  }
  return dup ? hashes[i] : 0;
}

static char* filetostr(const char* filename) {
  char* buffer = NULL;
  long  length;
  FILE* f = fopen(filename, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = _malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }
  return buffer;
}

void test_key_hash_collisions() {
  size_t    cap    = 10;
  uint16_t* hashes = _malloc(cap * sizeof(*hashes));
  TEST_ASSERT(hashes != NULL);

  const char* delim      = "\n";
  char*       keyfilestr = filetostr("/Users/sufi-al-hussaini/in3-core/src/core/client/keys.h");
  TEST_ASSERT_MESSAGE(keyfilestr != NULL, "File keys.h not found!");

  char* tok = strtok(keyfilestr, delim);
  int   i   = 0;
  for (; tok != NULL; i++, tok = strtok(NULL, delim)) {
    if (i == cap) {
      hashes = _realloc(hashes, cap * 2 * sizeof(*hashes), cap);
      cap *= 2;
    }
    char *kstr = substr(tok, "key(\"", "\")");
    if (kstr) {
      hashes[i] = key(kstr);
#ifdef DEBUG
      printf("\"%s\" => [%u]\n", kstr, hashes[i]);
#endif
    }
  }
  _free(keyfilestr);

  TEST_ASSERT_EQUAL(0, collision(hashes, i));
  _free(hashes);
}