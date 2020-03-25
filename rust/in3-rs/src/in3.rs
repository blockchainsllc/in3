//extern crate in3_sys;
use in3_sys::*;
use std::ffi;
use in3_sys::in3_ret_t;

pub struct Client {
    ptr: *mut in3_sys::in3_t,
}

pub struct Ctx {
    ptr: *mut in3_sys::in3_ctx_t,
}

impl Ctx {
    pub fn new(in3: &mut Client, config: String) -> Ctx {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            Ctx { ptr: in3_sys::ctx_new(in3.ptr, config_c.as_ptr())}
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
}

impl Request {
    pub fn new(ctx : &mut Ctx) -> Request {
        unsafe {
            Request { ptr: in3_sys::in3_create_request(ctx.ptr) }
        }
    }
}

impl Client {
    pub fn new() -> Client {
        unsafe {
            Client { ptr: in3_sys::in3_for_chain_auto_init(1) }
        }
    }
    // eth get balance with rpc call
    fn eth_get_balance_rpc() ->  String {
            let mut null: *mut i8 = std::ptr::null_mut();
            let mut res: *mut *mut i8 = &mut null;
            let err: *mut *mut i8 = &mut null;
            unsafe {
                let _ = in3_sys::in3_client_rpc(Client::new().ptr, ffi::CString::new("eth_getBalance").unwrap().as_ptr(),
                                                ffi::CString::new("[\"0xc94770007dda54cF92009BFF0dE90c06F603a09f\", \"latest\"]").unwrap().as_ptr(), res, err);
                // to view run with `cargo test -- --nocapture`
                ffi::CStr::from_ptr(*res).to_str().unwrap().to_string()
            }

    }
    fn eth_block_number(in3 : &mut Client) {
        unsafe {
            in3_sys::eth_blockNumber(in3.ptr);
        }
    }
    // in3 client config : TODO: all is self, we already have in3
    pub fn configure(&mut self, config: String) {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            in3_sys::in3_configure(self.ptr, config_c.as_ptr());
        }
    }
    pub fn execute(&self, ctx : &mut Ctx) -> in3_ret_t {
        unsafe {
            in3_sys::in3_ctx_execute(ctx.ptr)
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
    use std::ffi;

    use super::*;

    //#[test]
    fn test_eth_blknum() {
        let mut in3 = Client::new();
        Client::eth_block_number(&mut in3);
    }

    #[test]
    fn test_in3_config() {
        let mut in3 = Client::new();
        let mut config = String::from("{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
        let c = in3.configure(config);
    }
    
    #[test]
    fn test_in3_create_request() {
        let mut in3 = Client::new();
        let mut req_s = String::from(r#"{"method":"eth_blockNumber","params":[]}"#);
        let mut ctx = Ctx::new(&mut in3, req_s);
        let mut request = Request::new(&mut ctx);
        let _ = in3.execute(&mut ctx);

    }

    //#[test]
    fn test_eth_get_balance() {
            println!("------> balance: {}", Client::eth_get_balance_rpc());
    }
}
