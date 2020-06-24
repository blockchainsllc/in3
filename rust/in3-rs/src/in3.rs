//! IN3 client implementation.
use ffi::{CStr, CString};
use std::convert::TryInto;
use std::ffi;
use std::str;

use libc::{c_char, strlen};
use rustc_hex::FromHex;

use async_trait::async_trait;

use crate::error::{Error, In3Result};
use crate::signer;
use crate::traits::{Client as ClientTrait, Signer, Storage, Transport};
use crate::transport::HttpTransport;
use crate::types::Bytes;

/// Chain identifiers
pub mod chain {
    pub type ChainId = u32;

    /// Chain Id representing set of all supported chains
    pub const MULTICHAIN: u32 = 0x0;
    /// Chain Id for mainnet
    pub const MAINNET: u32 = 0x01;
    /// Chain Id for kovan
    pub const KOVAN: u32 = 0x2a;
    /// Chain Id for tobalaba
    pub const TOBALABA: u32 = 0x44d;
    /// Chain Id for goerli
    pub const GOERLI: u32 = 0x5;
    /// Chain Id for evan
    pub const EVAN: u32 = 0x4b1;
    /// Chain Id for IPFS
    pub const IPFS: u32 = 0x7d0;
    /// Chain Id for bitcoin
    pub const BTC: u32 = 0x99;
    /// Chain Id for local chains
    pub const LOCAL: u32 = 0xffff;
}

struct Ctx {
    ptr: *mut in3_sys::in3_ctx_t,
    #[allow(dead_code)]
    config: ffi::CString,
}

impl Ctx {
    fn new(in3: &mut Client, config_str: &str) -> Ctx {
        let config = ffi::CString::new(config_str).expect("CString::new failed");
        let ptr: *mut in3_sys::in3_ctx_t;
        unsafe {
            ptr = in3_sys::ctx_new(in3.ptr, config.as_ptr());
        }
        Ctx { ptr, config }
    }

    unsafe fn signc(&mut self, data: *const c_char, len: usize, pk_:*mut u8) -> Bytes {
        let pk = (*(*(*self.ptr).client).signer).wallet as *mut u8;
        signer::signc(pk, data, len)
    }

    async unsafe fn sign(&mut self, msg: Bytes) -> Bytes{
        let cptr = (*self.ptr).client;
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        let signer = &mut (*c).signer;
        let no_signer = signer.is_none();
        if no_signer {
            let wallet = &mut (*c).wallet;
            let pk = wallet.as_ref().unwrap();// cannot fail as wallet is string
            let pk_ = pk.as_str();
            let pk_data = pk_.as_ptr() as *mut u8;
            let c_data = msg.0.as_ptr() as *const c_char;
            let sig = self.signc(c_data, msg.0.len(), pk_data);
            return sig;
        } else if let Some(signer) = &mut (*c).signer {
            let sig = signer.sign(msg);
            return sig.await.expect("Signing failed");
        }
        Bytes::default()
    }

    async unsafe fn execute(&mut self) -> In3Result<String> {
        let mut last_waiting: *mut in3_sys::in3_ctx_t = std::ptr::null_mut();
        let mut p: *mut in3_sys::in3_ctx_t;
        p = self.ptr;
        match in3_sys::in3_ctx_execute(self.ptr) {
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
                    return Err("Cound not find the last waiting context".into());
                } else {
                    in3_sys::ctx_handle_failable(last_waiting);
                    return Err(Error::TryAgain);
                }
            }
            in3_sys::in3_ret_t::IN3_WAITING => {
                while p != std::ptr::null_mut() {
                    if (*p).raw_response == std::ptr::null_mut()
                        && in3_sys::in3_ctx_state(p) == in3_sys::state::CTX_WAITING_FOR_RESPONSE
                    {
                        last_waiting = p;
                    }
                    p = (*p).required;
                }
                if last_waiting == std::ptr::null_mut() {
                    return Err("Cound not find the last waiting context".into());
                }
            }
            in3_sys::in3_ret_t::IN3_OK => {
                let result = (*(*self.ptr).response_context).c;
                let data = ffi::CStr::from_ptr(result)
                    .to_str()
                    .expect("result is not valid UTF-8");
                return Ok(data.into());
            }
            err => {
                return Err(err.into());
            }
        }

        if last_waiting != std::ptr::null_mut() {
            let req = in3_sys::in3_create_request(last_waiting);
            let req_type = (*last_waiting).type_;
            match req_type {
                in3_sys::ctx_type::CT_SIGN => {
                    let ite_ = (*last_waiting).requests.offset(0);
                    let item_ = (*(*ite_)).data as *const c_char;
                    let slice = CStr::from_ptr(item_)
                        .to_str()
                        .expect("result is not valid UTF-8");
                    let request: serde_json::Value = serde_json::from_str(slice)
                        .expect("result not valid JSON");
                    let data_str = &request["params"][0].as_str().expect("params[0] not string");
                    let data_hex = data_str[2..].from_hex().expect("message is not valid hex string");
                    let mut res_str = self.sign(data_hex.into()).await;
                    in3_sys::in3_req_add_response(req, 0.try_into().unwrap(), false, res_str.0.as_mut_ptr() as *const c_char, 65);
                }
                in3_sys::ctx_type::CT_RPC => {
                    let payload = ffi::CStr::from_ptr((*req).payload).to_str()
                        .expect("payload is not valid UTF-8");
                    let urls_len = (*req).urls_len;
                    let mut urls = Vec::new();
                    for i in 0..urls_len as usize {
                        let url = ffi::CStr::from_ptr(*(*req).urls.add(i)).to_str()
                            .expect("URL is not valid UTF-8");
                        urls.push(url);
                    }

                    let responses: Vec<Result<String, String>> = {
                        let transport = {
                            let c = (*(*last_waiting).client).internal as *mut Client;
                            &mut (*c).transport
                        };
                        transport.fetch(payload, &urls).await
                    };
                    // println!("{:?}", responses);
                    for (i, resp) in responses.iter().enumerate() {
                        match resp {
                            Err(err) => {
                                let err_str = ffi::CString::new(err.to_string()).unwrap(); // cannot fail as err is string
                                in3_sys::in3_req_add_response(
                                    req,
                                    i.try_into().unwrap(), // cannot fail
                                    true,
                                    err_str.as_ptr(),
                                    -1i32,
                                );
                            }
                            Ok(res) => {
                                let res_str = ffi::CString::new(res.to_string()).unwrap(); // cannot fail as res is string
                                in3_sys::in3_req_add_response(
                                    req,
                                    i.try_into().unwrap(), // cannot fail
                                    false,
                                    res_str.as_ptr(),
                                    -1i32,
                                );
                            }
                        }
                    }
                    let res = *(*req).results.offset(0);
                    let mut err = Error::TryAgain;
                    if res.result.len == 0 {
                        let error = (*(*req).results.offset(0)).error;
                        err = ffi::CStr::from_ptr(error.data).to_str()
                            .expect("err is not valid UTF-8").into();
                    }
                    in3_sys::request_free(req, last_waiting, false);
                    return Err(err.into());
                }
            }
        }
        return Err(Error::TryAgain);
    }

    #[cfg(feature = "blocking")]
    fn send(&mut self) -> In3Result<()> {
        unsafe {
            let ret = in3_sys::in3_send_ctx(self.ptr);
            match ret {
                in3_sys::in3_ret_t::IN3_OK => Ok(()),
                _ => Err(ret.into()),
            }
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

/// Client struct
pub struct Client {
    /// Opaque handle to IN3 client
    /// Stored as a pointer to ensure type is `!Send`/`!Sync`
    ptr: *mut in3_sys::in3_t,
    /// Transport implementation
    transport: Box<dyn Transport>,
    /// Signer implementation
    signer: Option<Box<dyn Signer>>,
    /// Storage implementation
    storage: Option<Box<dyn Storage>>,
    /// private key wallet,
    wallet: Option<String>,
}

#[async_trait(? Send)]
impl ClientTrait for Client {
    /// Configures the IN3 client using a JSON str.
    ///
    /// # Example with supported options
    /// ```
    /// # use in3::prelude::*;
    ///
    /// let mut client = Client::new(chain::MAINNET);
    /// assert!(client.configure(r#"{
    /// 	"autoUpdateList": true,
    /// 	"signatureCount": 0,
    /// 	"finality": 0,
    /// 	"includeCode": false,
    /// 	"maxAttempts": 7,
    /// 	"keepIn3": false,
    /// 	"stats": true,
    /// 	"useBinary": false,
    /// 	"useHttp": false,
    /// 	"maxBlockCache": 0,
    /// 	"maxCodeCache": 0,
    /// 	"maxVerifiedHashes": 5,
    /// 	"timeout": 10000,
    /// 	"minDeposit": 0,
    /// 	"nodeProps": 0,
    /// 	"nodeLimit": 0,
    /// 	"proof": "standard",
    /// 	"requestCount": 1,
    /// 	"nodes": {
    /// 		"0x2a": {
    /// 			"contract": "0x4c396dcf50ac396e5fdea18163251699b5fcca25",
    /// 			"registryId": "0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf",
    /// 			"needsUpdate": true,
    /// 			"avgBlockTime": 6
    /// 		}
    /// 	}
    /// }"#).is_ok());
    /// ```
    fn configure(&mut self, config: &str) -> Result<(), String> {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            let err = in3_sys::in3_configure(self.ptr, config_c.as_ptr());
            if err.as_ref().is_some() {
                return Err(ffi::CStr::from_ptr(err).to_str().unwrap().to_string()); // cannot fail as err is guaranteed to be a C string
            }
        }
        Ok(())
    }

    /// Sets custom transport.
    ///
    /// # Example
    /// ```
    /// # use in3::prelude::*;
    /// use in3::transport::MockTransport;
    ///
    /// # let mut client = Client::new(chain::MAINNET);
    /// client.set_transport(Box::new(MockTransport {
    ///     responses: vec![(
    ///         "eth_blockNumber",
    ///         r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
    ///     )],
    /// }));
    /// ```
    fn set_transport(&mut self, transport: Box<dyn Transport>) {
        self.transport = transport;
    }

    /// Sets custom signer.
    ///
    /// # Example
    /// ```
    /// # use in3::prelude::*;
    /// use in3::signer::In3Signer;
    /// use std::convert::TryInto;
    ///
    /// # let mut client = Client::new(chain::MAINNET);
    /// client.set_signer(Box::new(In3Signer::new(
    ///         "8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f"
    ///         .try_into().unwrap())));
    /// ```
    fn set_signer(&mut self, signer: Box<dyn Signer>) {
        self.signer = Some(signer);
    }

    /// Sets custom storage.
    ///
    /// # Example
    /// ```
    /// # use in3::prelude::*;
    /// use std::collections::HashMap;
    ///
    /// struct MemStorage {
    ///     buf: HashMap<String, Vec<u8>>
    /// }
    ///
    /// impl Default for MemStorage {
    ///     fn default() -> Self {
    ///         MemStorage{buf: HashMap::new()}
    ///     }
    /// }
    ///
    /// impl Storage for MemStorage {
    ///     fn get(&self,key: &str) -> Option<Vec<u8>> {
    ///         Some(self.buf.get(key)?.clone())
    ///     }
    ///
    ///     fn set(&mut self,key: &str,value: &[u8]) {
    ///         self.buf.insert(key.to_string(), value.into());
    ///     }
    ///
    ///     fn clear(&mut self) {
    ///         self.buf.clear()
    ///     }
    /// }
    ///
    /// # let mut client = Client::new(chain::MAINNET);
    /// client.set_storage(Box::new(MemStorage::default()));
    /// ```
    fn set_storage(&mut self, storage: Box<dyn Storage>) {
        let no_storage = self.storage.is_none();
        self.storage = Some(storage);
        if no_storage {
            unsafe {
                (*self.ptr).cache = in3_sys::in3_set_storage_handler(
                    self.ptr,
                    Some(Client::in3_rust_storage_get),
                    Some(Client::in3_rust_storage_set),
                    Some(Client::in3_rust_storage_clear),
                    self.ptr as *mut libc::c_void,
                );
            }
        }
    }

    fn set_log_debug(&mut self){
        unsafe {
            in3_sys::in3_log_set_quiet_(0);
            in3_sys::in3_log_set_level_(in3_sys::in3_log_level_t::LOG_TRACE);
        }       
    }

    async fn rpc(&mut self, call: &str) -> In3Result<String> {
        let mut ctx = Ctx::new(self, call);
        loop {
            let res = unsafe { ctx.execute().await };
            if res != Err(Error::TryAgain) {
                return res;
            }
        }
    }

    #[cfg(feature = "blocking")]
    fn rpc_blocking(&mut self, call: &str) -> In3Result<String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        let req_str = ffi::CString::new(call).unwrap(); // cannot fail as call is a string
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr, req_str.as_ptr(), res, err);
            match ret {
                in3_sys::in3_ret_t::IN3_OK => {
                    Ok(ffi::CStr::from_ptr(*res).to_str()
                        .expect("result is not valid UTF-8").to_string())
                }
                _ => Err(Error::CustomError(
                    ffi::CStr::from_ptr(*err).to_str()
                        .expect("error is not valid UTF-8").to_string(),
                )),
            }
        }
    }

    fn set_pk_signer(&mut self, data: &str) {
        unsafe {
            self.wallet = Some(String::from(data));
            // let c_data = data.as_ptr() as *mut libc::c_char;
            // in3_sys::eth_set_pk_signer_hex(self.ptr, c_data);
            let pk_ = Client::hex_to_bytes(data);
            in3_sys::eth_set_pk_signer(self.ptr,  pk_);
            
            
        }
    }
}

impl Client {
    /// Constructs a new `Box<Client>` for specified chain Id.
    /// Defaults to `HttpTransport` with no signer and storage.
    ///
    /// # Example
    /// ```
    /// use in3::prelude::*;
    ///
    /// let client = Client::new(chain::MAINNET);
    /// ```
    pub fn new(chain_id: chain::ChainId) -> Box<Client> {
        unsafe {
            in3_sys::in3_log_set_quiet_(0);
            in3_sys::in3_log_set_level_(in3_sys::in3_log_level_t::LOG_TRACE);
            let mut c = Box::new(Client {
                ptr: in3_sys::in3_for_chain_auto_init(chain_id),
                transport: Box::new(HttpTransport {}),
                signer: None,
                storage: None,
                wallet: None,
            });
            let c_ptr: *mut ffi::c_void = &mut *c as *mut _ as *mut ffi::c_void;
            (*c.ptr).internal = c_ptr;
            #[cfg(feature = "blocking")]
                {
                    (*c.ptr).transport = Some(Client::in3_rust_transport);
                }
            c
        }
    }
    
    unsafe extern "C" fn in3_rust_storage_get(
        cptr: *mut libc::c_void,
        key: *const libc::c_char,
    ) -> *mut in3_sys::bytes_t {
        let key = ffi::CStr::from_ptr(key).to_str()
            .expect("URL is not valid UTF-8");
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &(*c).storage {
            if let Some(val) = storage.get(key) {
                return in3_sys::b_new(val.as_ptr(), val.len() as u32);
            }
        }
        std::ptr::null_mut()
    }

    unsafe extern "C" fn in3_rust_storage_set(
        cptr: *mut libc::c_void,
        key: *const libc::c_char,
        value: *mut in3_sys::bytes_t,
    ) {
        let key = ffi::CStr::from_ptr(key).to_str()
            .expect("key is not valid UTF-8");
        let value = std::slice::from_raw_parts_mut((*value).data, (*value).len as usize);
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &mut (*c).storage {
            storage.set(key, value);
        }
    }

    unsafe extern "C" fn in3_rust_storage_clear(cptr: *mut libc::c_void) {
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &mut (*c).storage {
            storage.clear();
        }
    }

    #[cfg(feature = "blocking")]
    extern "C" fn in3_rust_transport(
        request: *mut in3_sys::in3_request_t,
    ) -> in3_sys::in3_ret_t::Type {
        // internally calls the rust transport impl, i.e. Client.transport
        let mut urls = Vec::new();

        unsafe {
            let payload = ffi::CStr::from_ptr((*request).payload).to_str()
                .expect("URL is not valid UTF-8");
            let urls_len = (*request).urls_len;
            for i in 0..urls_len as usize {
                let url = ffi::CStr::from_ptr(*(*request).urls.add(i))
                    .to_str()
                    .expect("URL is not valid UTF-8");
                urls.push(url);
            }

            let c = (*(*request).in3).internal as *mut Client;
            let responses: Vec<Result<String, String>> =
                (*c).transport.fetch_blocking(payload, &urls);

            let mut any_err = false;
            for (i, resp) in responses.iter().enumerate() {
                match resp {
                    Err(err) => {
                        any_err = true;
                        let err_str = ffi::CString::new(err.to_string())
                            .expect("error is not valid UTF-8");
                        in3_sys::sb_add_chars(
                            &mut (*(*request).results.add(i)).error,
                            err_str.as_ptr(),
                        );
                    }
                    Ok(res) => {
                        let res_str = ffi::CString::new(res.to_string())
                            .expect("result is not valid UTF-8");
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

    unsafe fn hex_to_bytes(data: &str) -> *mut u8 {
        let c_str_data = CString::new(data).unwrap(); // cannot fail since data is a string
        let c_data: *const c_char = c_str_data.as_ptr();
        let mut dst = [0u8;65];
        let out:*mut u8 = dst.as_mut_ptr();
        let len: i32 = -1;
        in3_sys::hex_to_bytes(c_data, len, out, 32);
        out
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
}
