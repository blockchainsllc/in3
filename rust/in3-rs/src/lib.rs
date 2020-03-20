extern crate in3_sys;
use in3_sys::bytes_t;

use libc::{c_int, c_uint, c_void};
#[derive(Debug)]
pub struct In3 {
    data: bytes_t,
}

#[cfg(test)]
mod tests {
    #[test]
    fn eth_block_number() {
        unsafe {
            // in3_sys::in3_for_chain_auto_init(0);
            in3_sys::eth_blockNumber(in3_sys::in3_for_chain_auto_init(1));
        }
    }

}