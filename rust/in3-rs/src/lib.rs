extern crate in3_sys;

struct In3 {
    ptr: *mut in3_sys::in3_t,
}

impl In3 {
    fn new() -> In3 {
        unsafe {
            In3 { ptr: in3_sys::in3_for_chain_auto_init(1) }
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

    #[test]
    fn eth_block_number() {
        unsafe {
            // in3_sys::in3_for_chain_auto_init(0);
            in3_sys::eth_blockNumber(in3_sys::in3_for_chain_auto_init(1));
        }
    }

}