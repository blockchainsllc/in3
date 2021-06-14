#include "../../core/client/plugin.h"

bool handle_option(in3_t* c, char* key, char* value, sb_t* conf, int argc, char** argv);
bool handle_rpc(in3_t* c, char** method, sb_t* params, int argc, char** argv);
void init_env(in3_t* c, int argc, char* argv[]);
void init_recorder(int* argc, char*** argv);