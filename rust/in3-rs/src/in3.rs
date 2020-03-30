use std::ffi;

pub enum In3Ret {
    OK,
    EUNKNOWN,
    ENOMEM,
    ENOTSUP,
    EINVAL,
    EFIND,
    ECONFIG,
    ELIMIT,
    EVERS,
    EINVALDT,
    EPASS,
    ERPC,
    ERPCNRES,
    EUSNURL,
    ETRANS,
    ERANGE,
    WAITING,
    EIGNORE,
}

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
    pub fn new(in3: &mut Client, config_str: &'static str) -> Ctx {
        let config = ffi::CString::new(config_str).expect("CString::new failed");
        let mut ptr: *mut in3_sys::in3_ctx_t;
        unsafe {
            ptr = in3_sys::ctx_new(in3.ptr, config.as_ptr());
        }
        Ctx { ptr, config }
    }

    pub fn execute(&mut self) -> In3Ret {
        unsafe {
            let ret = in3_sys::in3_ctx_execute(self.ptr);
            let req = in3_sys::in3_create_request(self.ptr);
            let payload = ffi::CStr::from_ptr((*req).payload).to_str().unwrap();
            println!("{}, {}", payload, self.config.to_str().unwrap());
        }
        In3Ret::OK
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
    ctx_ptr: *mut in3_sys::in3_ctx_t,
}

impl Request {
    pub fn new(ctx: &mut Ctx) -> Request {
        unsafe {
            Request { ptr: in3_sys::in3_create_request(ctx.ptr), ctx_ptr: ctx.ptr }
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
}

impl Client {
    pub fn new(chain_id: chain::ChainId) -> Client {
        unsafe {
            let mut c = Client {
                ptr: in3_sys::in3_for_chain_auto_init(chain_id),
                transport: None,
            };
            c.set_transport(Box::new(crate::transport::transport_http));
            c
        }
    }

    pub fn set_transport(&mut self, transport: Box<dyn FnMut(&str, &[&str]) -> Vec<Result<String, String>>>) {
        self.transport = Some(transport);
        unsafe {
            (*self.ptr).transport = Some(Client::in3_rust_transport);
            self.update_internal();
        }
    }

    extern fn in3_rust_transport(client: *mut in3_sys::in3_t, request: *mut in3_sys::in3_request_t) -> in3_sys::in3_ret_t {
        // internally calls the rust transport impl, i.e. Client.transport
        let mut urls = Vec::new();

        unsafe {
            let payload = ffi::CStr::from_ptr((*request).payload).to_str().unwrap();
            let urls_len = (*request).urls_len;
            for i in 0..urls_len as usize {
                let url = ffi::CStr::from_ptr(*(*request).urls.add(i)).to_str().unwrap();
                urls.push(url);
            }

            let c = Client::get_internal(client);
            let responses: Vec<Result<String, String>> = match &mut (*c).transport {
                None => { panic!("Missing transport!") }
                Some(transport) => { (*transport)(payload, &urls) }
            };

            let mut any_err = false;
            for (i, resp) in responses.iter().enumerate() {
                match resp {
                    Err(err) => {
                        any_err = true;
                        let err_str = ffi::CString::new(err.to_string()).unwrap();
                        in3_sys::sb_add_chars(&mut (*(*request).results.add(i)).error, err_str.as_ptr());
                    }
                    Ok(res) => {
                        let res_str = ffi::CString::new(res.to_string()).unwrap();
                        in3_sys::sb_add_chars(&mut (*(*request).results.add(i)).result, res_str.as_ptr());
                    }
                }
            }

            if urls_len as usize != responses.len() || any_err {
                return in3_sys::in3_ret_t::IN3_ETRANS;
            }
        }

        in3_sys::in3_ret_t::IN3_OK
    }

    unsafe fn get_internal(ptr: *mut in3_sys::in3_t) -> *mut Client {
        (*ptr).internal as *mut Client
    }

    unsafe fn update_internal(&mut self) {
        let c_ptr: *mut ffi::c_void = self as *mut _ as *mut ffi::c_void;
        (*self.ptr).internal = c_ptr;
    }

    // in3 client config
    pub fn configure(&mut self, config: &str) -> Result<(), &'static str> {
        let config_str = ffi::CString::new(config).expect("config not readable");
        unsafe {
            let err = in3_sys::in3_configure(self.ptr, config_str.as_ptr());
            if err.as_ref().is_some() {
                return Err(ffi::CStr::from_ptr(err).to_str().unwrap());
            }
        }
        Ok(())
    }

    fn in3_ret_unwrap(&self, ret: in3_sys::in3_ret_t) -> In3Ret {
        match ret {
            in3_sys::in3_ret_t::IN3_OK => In3Ret::OK,
            in3_sys::in3_ret_t::IN3_ENOMEM => In3Ret::ENOMEM,
            in3_sys::in3_ret_t::IN3_EUNKNOWN => In3Ret::EUNKNOWN,
            in3_sys::in3_ret_t::IN3_ENOTSUP => In3Ret::ENOTSUP,
            in3_sys::in3_ret_t::IN3_EINVAL => In3Ret::EINVAL,
            in3_sys::in3_ret_t::IN3_EFIND => In3Ret::EFIND,
            in3_sys::in3_ret_t::IN3_ECONFIG => In3Ret::ECONFIG,
            in3_sys::in3_ret_t::IN3_ELIMIT => In3Ret::ELIMIT,
            in3_sys::in3_ret_t::IN3_EVERS => In3Ret::EVERS,
            in3_sys::in3_ret_t::IN3_EINVALDT => In3Ret::EINVALDT,
            in3_sys::in3_ret_t::IN3_EPASS => In3Ret::EPASS,
            in3_sys::in3_ret_t::IN3_ERPC => In3Ret::ERPC,
            in3_sys::in3_ret_t::IN3_ERPCNRES => In3Ret::ERPCNRES,
            in3_sys::in3_ret_t::IN3_EUSNURL => In3Ret::EUSNURL,
            in3_sys::in3_ret_t::IN3_ETRANS => In3Ret::ETRANS,
            in3_sys::in3_ret_t::IN3_ERANGE => In3Ret::ERANGE,
            in3_sys::in3_ret_t::IN3_WAITING => In3Ret::WAITING,
            in3_sys::in3_ret_t::IN3_EIGNORE => In3Ret::EIGNORE,
        }
    }

    pub fn send(&self, ctx: &mut Ctx) -> In3Ret {
        unsafe {
            self.in3_ret_unwrap(in3_sys::in3_send_ctx(ctx.ptr))
        }
    }

    pub fn rpc(&mut self, request: &str) -> Result<String, String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        let req_str = ffi::CString::new(request).unwrap();
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr,
                                                  req_str.as_ptr(),
                                                  res, err);
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
