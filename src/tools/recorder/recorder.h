#include "../../core/client/client.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include <stdarg.h>
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
void           recorder_write_start(in3_t* c, char* file, int argc, char* argv[]);
void           recorder_read_start(in3_t* c, char* file);
void           recorder_update_cmd(char* file, int* argc, char** argv[]);
void           recorder_print(int err, const char*, ...);
void           recorder_request(char* req);
void           recorder_response(char* req);
void           recorder_error(char* req);
void           recorder_configure(char* conf);
_Noreturn void recorder_exit(int code);
