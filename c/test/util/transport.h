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

#ifndef IN3_TRANSPORT_H
#define IN3_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../src/core/client/plugin.h"
void add_response(char* request_method, char* request_params, char* result, char* error, char* in3);
int  add_response_test(char* test, char* needed_params);

in3_ret_t mock_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);
in3_ret_t test_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

static inline in3_ret_t register_transport(in3_t* c, in3_plugin_act_fn fn) {
  return in3_plugin_register(c, PLGN_ACT_TRANSPORT_SEND | PLGN_ACT_TRANSPORT_RECEIVE | PLGN_ACT_TRANSPORT_CLEAN, fn, NULL, true);
}

static inline void replace_transport(in3_t* c, in3_plugin_act_fn custom_transport) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_TRANSPORT_SEND) {
      p->action_fn = custom_transport;
      return;
    }
  }
  in3_plugin_register(c, PLGN_ACT_TRANSPORT_SEND | PLGN_ACT_TRANSPORT_RECEIVE | PLGN_ACT_TRANSPORT_CLEAN, custom_transport, NULL, true);
}

#ifdef __cplusplus
}
#endif

#endif
