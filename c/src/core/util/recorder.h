#include "../client/client.h"
#include "data.h"
#include "debug.h"
#include "log.h"
#include "mem.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void recorder_write_start(in3_t* c, char* file, int argc, char* argv[]);
void recorder_read_start(in3_t* c, char* file);
void recorder_update_cmd(char* file, int* argc, char** argv[]);