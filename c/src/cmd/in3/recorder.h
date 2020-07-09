#include "../../core/client/client.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void recorder_write_start(in3_t* c, char* file);
void recorder_read_start(in3_t* c, char* file);
