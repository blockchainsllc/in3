use std::ffi;

use in3_sys::*;

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
    fn eth_getBalance() -> String {
        let mut null: *mut i8 = std::ptr::null_mut();
        let mut res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        unsafe {
            let _ = in3_sys::in3_client_rpc(Client::new(ChainId::Mainnet).ptr, ffi::CString::new("eth_getBalance").unwrap().as_ptr(),
                                            ffi::CString::new("[\"0xc94770007dda54cF92009BFF0dE90c06F603a09f\", \"latest\"]").unwrap().as_ptr(), res, err);
            // to view run with `cargo test -- --nocapture`
            ffi::CStr::from_ptr(*res).to_str().unwrap().to_string()
        }
    }

    fn eth_blockNumber(in3: &mut Client) {
        unsafe {
            in3_sys::eth_blockNumber(in3.ptr);
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

    #[test]
    fn test_eth_block_number() {
        let mut in3 = Client::new(ChainId::Mainnet);
        Client::eth_blockNumber(&mut in3);
    }

    #[test]
    fn test_eth_get_balance() {
        println!("------> balance: {}", Client::eth_getBalance());
    }
}
