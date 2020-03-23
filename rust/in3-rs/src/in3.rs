use std::ffi;

struct Client {
    ptr: *mut in3_sys::in3_t,
}

enum ChainId {
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

trait ClientNew<T> {
    fn new(_: T) -> Client;
}

impl ClientNew<u32> for Client {
    fn new(chain_id: u32) -> Client {
        unsafe {
            Client { ptr: in3_sys::in3_for_chain_auto_init(chain_id) }
        }
    }
}

impl ClientNew<ChainId> for Client {
    fn new(chain_id: ChainId) -> Client {
        Client::new(chain_id as u32)
    }
}

impl Client {
    fn rpc(&self, request: &str) -> Result<String, String> {
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
}
