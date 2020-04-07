use std::ffi;

use crate::error::In3Result;

pub mod chain {
    pub type ChainId = u32;

    pub const MULTICHAIN: u32 = 0x0;
    pub const MAINNET: u32 = 0x01;
    pub const KOVAN: u32 = 0x2a;
    pub const TOBALABA: u32 = 0x44d;
    pub const GOERLI: u32 = 0x5;
    pub const EVAN: u32 = 0x4b1;
    pub const IPFS: u32 = 0x7d0;
    pub const BTC: u32 = 0x99;
    pub const LOCAL: u32 = 0xffff;
}

pub struct Ctx {
    ptr: *mut in3_sys::in3_ctx_t,
    config: ffi::CString,
}

impl Ctx {
    pub fn new(in3: &mut Client, config_str: &str) -> Ctx {
        let config = ffi::CString::new(config_str).expect("CString::new failed");
        let ptr: *mut in3_sys::in3_ctx_t;
        unsafe {
            ptr = in3_sys::ctx_new(in3.ptr, config.as_ptr());
        }
        Ctx { ptr, config }
    }

    pub async fn execute(&mut self) -> In3Result<String> {
        unsafe {
            let mut ctx_ret;
            let ret = loop {
                ctx_ret = in3_sys::in3_ctx_execute(self.ptr);
                let mut last_waiting: *mut in3_sys::in3_ctx_t;
                let mut p: *mut in3_sys::in3_ctx_t;
                last_waiting = (*self.ptr).required;
                p = self.ptr;
                match ctx_ret {
                    in3_sys::in3_ret_t::IN3_EIGNORE => {
                        while p != std::ptr::null_mut() {
                            let p_req = (*p).required;
                            if p_req != std::ptr::null_mut()
                                && (*p_req).verification_state == in3_sys::in3_ret_t::IN3_EIGNORE
                            {
                                last_waiting = p;
                            }
                            p = (*last_waiting).required;
                        }
                        if last_waiting == std::ptr::null_mut() {
                            break Err("Cound not find the last waiting context");
                        } else {
                            in3_sys::ctx_handle_failable(last_waiting);
                        }
                    }
                    in3_sys::in3_ret_t::IN3_WAITING => {
                        while p != std::ptr::null_mut() {
                            let state = in3_sys::in3_ctx_state(p);
                            let res = (*p).raw_response;
                            if res == std::ptr::null_mut()
                                && state == in3_sys::state::CTX_WAITING_FOR_RESPONSE
                            {
                                last_waiting = p;
                            }
                            p = (*last_waiting).required;
                        }
                        if last_waiting == std::ptr::null_mut() {
                            break Err("Cound not find the last waiting context");
                        }
                    }
                    in3_sys::in3_ret_t::IN3_OK => {
                        let result = (*(*self.ptr).response_context).c;
                        let data = ffi::CStr::from_ptr(result).to_str().unwrap();
                        println!("{}", data);
                        break Ok(data);
                    }
                    _ => {
                        break Err("EIGNORE");
                    }
                }

                if last_waiting != std::ptr::null_mut() {
                    let req_type = (*last_waiting).type_;
                    match req_type {
                        in3_sys::ctx_type::CT_SIGN => {
                            println!("TODO CT_SIGN");
                            break Ok("TODO");
                        }
                        in3_sys::ctx_type::CT_RPC => {
                            let req = in3_sys::in3_create_request(last_waiting);
                            let payload = ffi::CStr::from_ptr((*req).payload).to_str().unwrap();
                            println!("{}, {}", payload, self.config.to_str().unwrap());
                            let in3_transport = (*(*self.ptr).client).transport.unwrap();
                            in3_transport((*self.ptr).client, req);
                            let result = (*(*req).results.offset(0)).result;
                            let len = result.len;
                            let data = ffi::CStr::from_ptr(result.data).to_str().unwrap();
                            println!("{}", data);
                            if len != 0 {
                                break Ok(data);
                            } else {
                                let error = (*(*req).results.offset(0)).error;
                                let err = ffi::CStr::from_ptr(error.data).to_str().unwrap();
                                break Err(err);
                            }
                        }
                    }
                }
            };
            let ret_str: String = ret.unwrap().to_owned();
            Ok(ret_str)
        }
    }
}

impl Drop for Ctx {
    fn drop(&mut self) {
        unsafe {
            in3_sys::ctx_free(self.ptr);
        }
    }
}

pub struct Request {
    ptr: *mut in3_sys::in3_request_t,
    ctx_ptr: *const in3_sys::in3_ctx_t,
}

impl Request {
    pub fn new(ctx: &mut Ctx) -> Request {
        unsafe {
            Request {
                ptr: in3_sys::in3_create_request(ctx.ptr),
                ctx_ptr: ctx.ptr,
            }
        }
    }
}

impl Drop for Request {
    fn drop(&mut self) {
        unsafe {
            in3_sys::request_free(self.ptr, self.ctx_ptr, false);
        }
    }
}

pub struct Client {
    ptr: *mut in3_sys::in3_t,
    transport: Option<Box<dyn FnMut(&str, &[&str]) -> Vec<Result<String, String>>>>,
    storage_get: Option<Box<dyn FnMut(&str) -> Vec<u8>>>,
    storage_set: Option<Box<dyn FnMut(&str, &[u8])>>,
    storage_clear: Option<Box<dyn FnMut()>>,
}

impl Client {
    pub async fn send_request(&mut self, config_str: &str) -> In3Result<String> {
        let mut ctx = Ctx::new(self, config_str);
        let _res = ctx.execute().await;
        _res
    }

    pub fn send(&self, ctx: &mut Ctx) -> In3Result<()> {
        unsafe {
            let ret = in3_sys::in3_send_ctx(ctx.ptr);
            match ret {
                in3_sys::in3_ret_t::IN3_OK => Ok(()),
                _ => Err(ret.into()),
            }
        }
    }

    pub fn new(chain_id: chain::ChainId) -> Box<Client> {
        unsafe {
            let mut c = Box::new(Client {
                ptr: in3_sys::in3_for_chain_auto_init(chain_id),
                transport: None,
                storage_get: None,
                storage_set: None,
                storage_clear: None,
            });
            let c_ptr: *mut ffi::c_void = &mut *c as *mut _ as *mut ffi::c_void;
            (*c.ptr).internal = c_ptr;
            (*c.ptr).cache = in3_sys::in3_set_storage_handler(c.ptr, Some(Client::in3_rust_storage_get),
                                                              Some(Client::in3_rust_storage_set),
                                                              Some(Client::in3_rust_storage_clear),
                                                              c.ptr as *mut libc::c_void);
            (*c.ptr).transport = Some(Client::in3_rust_transport);
            c.set_transport(Box::new(crate::transport::transport_http));
            c
        }
    }

    pub fn set_transport(&mut self, transport: Box<dyn FnMut(&str, &[&str]) -> Vec<Result<String, String>>>) {
        self.transport = Some(transport);
    }

    pub fn set_storage(&mut self, get: Box<dyn FnMut(&str) -> Vec<u8>>,
                       set: Box<dyn FnMut(&str, &[u8])>,
                       clear: Box<dyn FnMut()>) {
        self.storage_get = Some(get);
        self.storage_set = Some(set);
        self.storage_clear = Some(clear);
    }

    unsafe extern fn in3_rust_storage_get(cptr: *mut libc::c_void, key: *const libc::c_char) -> *mut in3_sys::bytes_t {
        let key = ffi::CStr::from_ptr(key).to_str().unwrap();
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        let val: Option<Vec<u8>> = match &mut (*c).storage_get {
            None => { None }
            Some(get) => { Some((*get)(key)) }
        };
        match val {
            Some(val) => in3_sys::b_new(val.as_ptr(), val.len() as u32),
            None => std::ptr::null_mut()
        }
    }

    unsafe extern fn in3_rust_storage_set(cptr: *mut libc::c_void, key: *const libc::c_char, value: *mut in3_sys::bytes_t) {
        let key = ffi::CStr::from_ptr(key).to_str().unwrap();
        let value = std::slice::from_raw_parts_mut((*value).data, (*value).len as usize);
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        match &mut (*c).storage_set {
            None => { None }
            Some(set) => { Some((*set)(key, value)) }
        };
    }

    unsafe extern fn in3_rust_storage_clear(cptr: *mut libc::c_void) {
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        match &mut (*c).storage_clear {
            None => { None }
            Some(clear) => { Some((*clear)()) }
        };
    }

    extern fn in3_rust_transport(client: *mut in3_sys::in3_t, request: *mut in3_sys::in3_request_t) -> in3_sys::in3_ret_t::Type {
        // internally calls the rust transport impl, i.e. Client.transport
        let mut urls = Vec::new();

        unsafe {
            let payload = ffi::CStr::from_ptr((*request).payload).to_str().unwrap();
            let urls_len = (*request).urls_len;
            for i in 0..urls_len as usize {
                let url = ffi::CStr::from_ptr(*(*request).urls.add(i))
                    .to_str()
                    .unwrap();
                urls.push(url);
            }

            let c = (*client).internal as *mut Client;
            let responses: Vec<Result<String, String>> = match &mut (*c).transport {
                None => panic!("Missing transport!"),
                Some(transport) => (*transport)(payload, &urls),
            };

            let mut any_err = false;
            for (i, resp) in responses.iter().enumerate() {
                match resp {
                    Err(err) => {
                        any_err = true;
                        let err_str = ffi::CString::new(err.to_string()).unwrap();
                        in3_sys::sb_add_chars(
                            &mut (*(*request).results.add(i)).error,
                            err_str.as_ptr(),
                        );
                    }
                    Ok(res) => {
                        let res_str = ffi::CString::new(res.to_string()).unwrap();
                        in3_sys::sb_add_chars(
                            &mut (*(*request).results.add(i)).result,
                            res_str.as_ptr(),
                        );
                    }
                }
            }

            if urls_len as usize != responses.len() || any_err {
                return in3_sys::in3_ret_t::IN3_ETRANS;
            }
        }

        in3_sys::in3_ret_t::IN3_OK
    }

    // in3 client config
    pub fn configure(&mut self, config: &str) -> Result<(), String> {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            let err = in3_sys::in3_configure(self.ptr, config_c.as_ptr());
            if err.as_ref().is_some() {
                return Err(ffi::CStr::from_ptr(err).to_str().unwrap().to_string());
            }
        }
        Ok(())
    }

    pub fn rpc(&mut self, request: &str) -> Result<String, String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        let req_str = ffi::CString::new(request).unwrap();
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr, req_str.as_ptr(), res, err);
            return if ret == in3_sys::in3_ret_t::IN3_OK {
                Ok(ffi::CStr::from_ptr(*res).to_str().unwrap().to_string())
            } else {
                Err(ffi::CStr::from_ptr(*err).to_str().unwrap().to_string())
            };
        }
    }
}

impl Drop for Client {
    fn drop(&mut self) {
        unsafe {
            in3_sys::in3_free(self.ptr);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_in3_config() {
        let mut in3 = Client::new(chain::MAINNET);
        let c = in3.configure(r#"{"autoUpdateList":false}"#);
        assert_eq!(c.is_err(), false);
    }

    #[test]
    fn test_in3_create_request() {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method":"eth_blockNumber","params":[]}"#);
        let _request = Request::new(&mut ctx);
        let _ = ctx.execute();
    }
}
