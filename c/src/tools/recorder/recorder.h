#include "../../core/client/client.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** Record runs of client in a file to reproduce errors.
 */
void in3_record(
    in3_t* c,    /**< the incubed client */
    char*  file, /**< file to read or write to record run (max file name length 31)  */
    bool   in    /**< indicates wether to read a recorded run or start a new recording */
);
void init_recorder(in3_t* c, char* file);
void recorder_write_start(in3_t* c, char* file, int argc, char* argv[]);
void recorder_read_start(in3_t* c, char* file);
void recorder_update_cmd(char* file, int* argc, char** argv[]);