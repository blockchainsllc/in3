#ifndef _http_server_h___
#define _http_server_h___

#include "../../core/client/context.h"
#include <stdio.h>
#include <string.h>

void http_run_server(const char* port, in3_t* in3);

#endif
