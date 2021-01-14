import ctypes as c

from in3.libin3.enum import PluginAction, RPCCode

"""
/**
 * plugin interface definition
 */
typedef struct in3_plugin in3_plugin_t;

/**
 * plugin action handler
 *
 * Implementations of this function must strictly follow the below pattern for return values -
 * * IN3_OK - successfully handled specified action
 * * IN3_WAITING - handling specified action, but waiting for more information
 * * IN3_EIGNORE - could handle specified action, but chose to ignore it so maybe another handler could handle it
 * * Other errors - handled but failed
 */
typedef in3_ret_t (*in3_plugin_act_fn)(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

typedef uint32_t in3_plugin_supp_acts_t;

struct in3_plugin {
  in3_plugin_supp_acts_t acts;      /**< bitmask of supported actions this plugin can handle */
  void*                  data;      /**< opaque pointer to plugin data */
  in3_plugin_act_fn      action_fn; /**< plugin action handler */
  in3_plugin_t*          next;      /**< pointer to next plugin in list */
};
"""


class NativeIn3Plugin(c.Structure):
    """
    Based on in3/client/.h in3_plugin struct
    """
    _fields_ = [("acts", c.c_uint32),
                ("data", c.c_void_p),
                # in3_ret_t
                ("action_fn", c.c_int32),
                ("next", c.c_void_p)]


class In3Plugin:
    def __init__(self, actions: PluginAction, plugin_data: str = None, next_plugin_instance: int = None):
        self.acts = actions
        self.data = plugin_data
        self.next = next_plugin_instance
        self.action_fn = self.action_fn_wrapper

    @c.CFUNCTYPE(c.c_int32, c.c_void_p, c.c_int32, c.c_void_p)
    def action_fn_wrapper(self, plugin_data, action, plugin_ctx=None) -> RPCCode:
        # typedef in3_ret_t (*in3_plugin_act_fn)(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);
        return self.handler(plugin_data, PluginAction(action), plugin_ctx)

    def handler(self, plugin_data, action: PluginAction, plugin_ctx=None) -> RPCCode:
        raise NotImplementedError
        # return RPCCode.IN3_OK

    def register_plugin(self, instance: int, replace_old: bool = True):
        from in3.libin3.rpc_api import libin3_register_plugin
        error_code = RPCCode(libin3_register_plugin(instance, self.acts, self.action_fn, self.data, replace_old))
        if not error_code == RPCCode.IN3_OK:
            raise Exception("In3 Plugin failed to be registered with error: ", error_code)
