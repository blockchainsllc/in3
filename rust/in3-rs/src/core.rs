use std::ffi;

pub struct Client {
    ptr: *mut in3_sys::in3_t,
    transport: Option<Box<dyn FnMut(&str, &[&str]) -> Vec<Result<String, String>>>>,
}

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

pub trait ClientNew<T> {
    fn new(_: T) -> Client;
}

impl ClientNew<u32> for Client {
    fn new(chain_id: u32) -> Client {
        unsafe {
            let mut c = Client {
                ptr: in3_sys::in3_for_chain_auto_init(chain_id),
                transport: None,
            };
            c
        }
    }
}

impl ClientNew<ChainId> for Client {
    fn new(chain_id: ChainId) -> Client {
        Client::new(chain_id as u32)
    }
}

impl Client {
    pub fn set_auto_update_nodelist(&mut self, auto_update: bool) {
        unsafe {
            if auto_update {
                (*self.ptr).flags |= (1u8 >> in3_sys::in3_flags_type_t::FLAGS_AUTO_UPDATE_LIST as u8);
            } else {
                (*self.ptr).flags &= !(1u8 >> in3_sys::in3_flags_type_t::FLAGS_AUTO_UPDATE_LIST as u8);
            }
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
                        in3_sys::sb_add_chars(&mut (*(*request).results.add(i)).error, ffi::CString::new(err.to_string()).unwrap().as_ptr());
                    }
                    Ok(res) => {
                        in3_sys::sb_add_chars(&mut (*(*request).results.add(i)).result, ffi::CString::new(res.to_string()).unwrap().as_ptr());
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
