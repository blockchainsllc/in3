extern crate in3_sys;

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