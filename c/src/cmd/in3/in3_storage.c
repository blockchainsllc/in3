
/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "in3_storage.h"
#include "../../core/client/client.h"
#include "../../core/util/mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <dirent.h>
#else
#include <ftw.h>
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
      size_t new_alloc = allocated * 2 + 1;
      buffer           = _realloc(buffer, new_alloc, allocated);
      allocated        = new_alloc;
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

#if defined(_WIN32)
static void rm_recurs(const char* path) {
  struct dirent* entry = NULL;
  DIR*           dir   = NULL;
  dir                  = opendir(path);
  while ((entry = readdir(dir))) {
    DIR*  sub_dir       = NULL;
    FILE* file          = NULL;
    char  abs_path[270] = {0};
    if (*(entry->d_name) != '.') {
      sprintf(abs_path, "%s/%s", path, entry->d_name);
      if ((sub_dir = opendir(abs_path))) {
        closedir(sub_dir);
        rm_recurs(abs_path);
      } else {
        if ((file = fopen(abs_path, "r"))) {
          fclose(file);
          remove(abs_path);
        }
      }
    }
  }
  remove(path);
}
#else
static int unlink_cb(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
  UNUSED_VAR(sb);
  UNUSED_VAR(typeflag);
  UNUSED_VAR(ftwbuf);
  return remove(fpath);
}

static void rm_recurs(const char* dir) {
  nftw(dir, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
#endif

void storage_clear(void* cptr) {
  UNUSED_VAR(cptr);
  rm_recurs(get_storage_dir());
  // recreate storage dir
  free(_HOME_DIR);
  _HOME_DIR = NULL;
  get_storage_dir();
}
