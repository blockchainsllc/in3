
#ifndef MAIN_HELPER_H
#define MAIN_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/version.h"
#include "../../core/util/bitset.h"
#include "../../core/util/colors.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../tools/recorder/recorder.h"
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Noreturn void die(char* msg);

void print_hex(uint8_t* data, int len);

void read_pass(char* pw, int pwsize);

void configure(in3_t* c, char* name, char* value);
bool configure_arg(in3_t* c, char** args, int* index, int argc);

char* get_wei(char* val);

char* resolve(in3_t* c, char* name);

bytes_t readFile(FILE* f);

bytes_t* get_std_in();

void read_pk(char* pk_file, char* pwd, in3_t* c, char* method);

void print_val(d_token_t* t);

char*       get_argument(int argc, char* argv[], char* alias, char* arg, bool has_value);
void        add_argument(char* arg, sb_t* args, in3_t* c, char* method);
uint32_t*   get_output_conf();
const char* get_help_args();
typedef enum output {
  out_human = 1,
  out_hex   = 2,
  out_json  = 4,
  out_eth   = 8,
  out_debug = 16
} output_t;

void display_result(char* method, char* result);
#ifdef __cplusplus
}
#endif

#endif
