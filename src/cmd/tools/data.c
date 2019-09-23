/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 slock.it GmbH, Blockchains LLC
 * Authors Simon Jentzsch<simon@slock.it>
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
 *  
 *******************************************************************************/

/** @file 
 * simple commandline-util parsing json and creating bin
 * */

#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_from_stdin(FILE* file) {
  if (file == NULL) {
    printf("File not found!");
    _Exit(1);
    return NULL;
  }

  size_t   allocated = 1024;
  size_t   len       = 0;
  uint8_t* buffer    = _malloc(1025);
  size_t   r;

  while (1) {
    r = fread(buffer + len, 1, allocated - len, file);
    len += r;
    if (feof(file)) break;
    buffer = _realloc(buffer, allocated * 2 + 1, allocated);
    allocated *= 2;
  }

  buffer[len] = 0;
  if (file != stdin) fclose(file);
  return (char*) buffer;
}

int main(int argc, char* argv[]) {
  char* default_format = "hex";
  char* input          = NULL;
  char* format         = default_format;

  int i;

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0)
      input = read_from_stdin(fopen(argv[++i], "r"));
    else if (strcmp(argv[i], "-o") == 0)
      format = argv[++i];
    else
      input = argv[i];
  }

  if (input == NULL) input = read_from_stdin(stdin);

  json_ctx_t* ctx = parse_json(input);
  if (!ctx)
    printf("Invalid Json : %s\n", input);
  else {
    bytes_builder_t* bb = bb_new();
    d_serialize_binary(bb, ctx->result);

    if (strcmp(format, "hex") == 0) {
      for (i = 0; i < (int) bb->b.len; i++) printf("%02x", bb->b.data[i]);
      printf("\n");
    } else if (strcmp(format, "cstr") == 0) {
      unsigned char c = 0, is_hex = 0;

      for (i = 0; i < (int) bb->b.len; i++) {
        c      = bb->b.data[i];
        is_hex = c < ' ' || c > 0x7E || c == 0x5C || c == '"' || (is_hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')));
        printf(is_hex ? "\\x%02x" : "%c", c);
      }
      printf("\n len = %u\n", bb->b.len);
    } else {
      printf("unsuported output format %s!\n", format);
      return 1;
    }
  }

  return 0;
}