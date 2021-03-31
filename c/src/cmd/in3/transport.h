#include "../../core/client/plugin.h"

in3_ret_t debug_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

void init_transport(in3_t* c);
bool set_test_transport(in3_t* c, char* name);
bool set_onlyshow_rawtx();
bool is_onlyshow_rawtx();
bool set_response_file(bool is_in);
void check_last_output();