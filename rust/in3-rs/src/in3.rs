//extern crate in3_sys;
use in3_sys::*;
use std::ffi;
use crate::context::*;
use in3_sys::in3_ret_t;

struct In3 {
    ptr: *mut in3_sys::in3_t,
}

struct Ctx {
    ptr: *mut in3_sys::in3_ctx_t,
}

impl Ctx {
    fn new(in3: &mut In3, config: String) -> Ctx {
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

struct Request {
    ptr: *mut in3_sys::in3_request_t,
}

impl Request {
    fn new(ctx : &mut Ctx) -> Request {
        unsafe {
            Request { ptr: in3_sys::in3_create_request(ctx.ptr) }
        }
    }
}

impl In3 {
    fn new() -> In3 {
        unsafe {
            In3 { ptr: in3_sys::in3_for_chain_auto_init(1) }
        }
    }
    // eth get balance with rpc call
    fn eth_get_balance_rpc() ->  String {
            let mut null: *mut i8 = std::ptr::null_mut();
            let mut res: *mut *mut i8 = &mut null;
            let err: *mut *mut i8 = &mut null;
            unsafe {
                let _ = in3_sys::in3_client_rpc(In3::new().ptr, ffi::CString::new("eth_getBalance").unwrap().as_ptr(),
                                                ffi::CString::new("[\"0xc94770007dda54cF92009BFF0dE90c06F603a09f\", \"latest\"]").unwrap().as_ptr(), res, err);
                // to view run with `cargo test -- --nocapture`
                ffi::CStr::from_ptr(*res).to_str().unwrap().to_string()
            }

    }
    fn eth_block_number(in3 : &mut In3) {
        unsafe {
            in3_sys::eth_blockNumber(in3.ptr);
        }
    }
    // in3 client config
    fn configure(in3 : &mut In3, config: String) {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            in3_sys::in3_configure(in3.ptr, config_c.as_ptr());
        }
    }
    fn execute(ctx : &mut Ctx) -> in3_ret_t {
        unsafe {
            in3_sys::in3_ctx_execute(ctx.ptr)
        }
    }



}

impl Drop for In3 {
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
        let mut in3 = In3::new();
        In3::eth_blockNumber(&mut in3);
    }

    #[test]
    fn test_in3_config() {
        let mut in3 = In3::new();
        let mut config = String::from("{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
        let c = In3::configure(&mut in3, config);
    }
    
    #[test]
    fn test_in3_create_request() {
        let mut in3 = In3::new();
        let mut req_s = String::from("{\"method\":\"eth_blockNumber\",\"params\":[]}");
        let mut ctx = Ctx::new(&mut in3, req_s);
        let mut request = Request::new(&mut ctx);
    }

    //#[test]
    fn test_eth_get_balance() {
            println!("------> balance: {}", In3::eth_get_balance_rpc());
    }
}
