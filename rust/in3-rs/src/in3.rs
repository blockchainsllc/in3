//extern crate in3_sys;
use in3_sys::*;
use std::ffi;
use in3_sys::in3_ret_t;

pub enum ChainId {
    Multichain = 0x0,
    Mainnet = 0x01,
    Kovan = 0x2a,
    Tobalaba = 0x44d,
    Goerli = 0x5,
    Evan = 0x4b1,
    Ipfs = 0x7d0,
    Btc = 0x99,
    Local = 0xffff,
}
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
//TODO:implement drpo for Request, need ctx param 
// impl Drop for Request {
//     fn drop(&mut self) {
//         unsafe {
//             in3_sys::request_free(self.ptr);
//         }
//     }
// }

impl Request {
    pub fn new(ctx : &mut Ctx) -> Request {
        unsafe {
            Request { ptr: in3_sys::in3_create_request(ctx.ptr) }
        }
    }
}


impl Client {

    pub fn new(chain_id: ChainId) -> Client {
        unsafe {
            Client { ptr: in3_sys::in3_for_chain_auto_init(chain_id as u32) }
        }
    }

    // eth get balance with rpc call
    fn eth_get_balance_rpc() ->  String {
            let mut null: *mut i8 = std::ptr::null_mut();
            let res: *mut *mut i8 = &mut null;
            let err: *mut *mut i8 = &mut null;
            unsafe {
                let _ = in3_sys::in3_client_rpc(Client::new(ChainId::Mainnet).ptr, ffi::CString::new("eth_getBalance").unwrap().as_ptr(),
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
    fn ctx_unwrap(&self, ret: in3_sys::in3_ret_t) -> In3Ret{
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
    pub fn execute(&self, ctx : &mut Ctx) -> In3Ret {
        unsafe {
            self.ctx_unwrap(in3_sys::in3_ctx_execute(ctx.ptr))
        }
    }

    pub fn send(&self, ctx : &mut Ctx) -> In3Ret {
        unsafe {
            self.ctx_unwrap(in3_sys::in3_send_ctx(ctx.ptr))
        }
    }
    pub fn rpc(&self, request: &str) -> Result<String, String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr,
                                                  ffi::CString::new(request).unwrap().as_ptr(),
                                                  res, err);
            return if ret == in3_sys::in3_ret_t::IN3_OK {
                Ok(ffi::CStr::from_ptr(*res).to_str().unwrap().to_string())
            } else {
                Err(ffi::CStr::from_ptr(*err).to_str().unwrap().to_string())
            };
        }
    }
    pub async fn arpc(&self, request: &str) -> Result<String, String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr,
                                                  ffi::CString::new(request).unwrap().as_ptr(),
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
    use std::ffi;

    use super::*;

    //#[test]
    fn test_eth_blknum() {
        let mut in3 = Client::new(ChainId::Mainnet);
        Client::eth_block_number(&mut in3);
    }

    #[test]
    fn test_in3_config() {
        let mut in3 = Client::new(ChainId::Mainnet);
        let mut config = String::from("{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
        let c = in3.configure(config);
    }
    
    #[test]
    fn test_in3_create_request() {
        let mut in3 = Client::new(ChainId::Mainnet);
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
