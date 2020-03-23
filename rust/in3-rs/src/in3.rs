//extern crate in3_sys;
use in3_sys::*;
use std::ffi;
struct In3 {
    ptr: *mut in3_sys::in3_t,
}


impl In3 {
    fn new() -> In3 {
        unsafe {
            In3 { ptr: in3_sys::in3_for_chain_auto_init(1) }
        }
    }
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
    fn eth_blockNumber(in3 : &mut In3) {
        unsafe {
            in3_sys::eth_blockNumber(in3.ptr);
        }
    }

    fn configure(in3 : &mut In3, config: String) {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            in3_sys::in3_configure(in3.ptr, config_c.as_ptr());
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

    #[test]
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
    fn test_eth_get_balance() {
            println!("------> balance: {}", In3::eth_get_balance_rpc());
    }
}
