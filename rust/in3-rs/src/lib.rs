extern crate in3_sys;
use in3_sys::bytes_t;

use libc::{c_int, c_uint, c_void};
#[derive(Debug)]
pub struct In3 {
    data: bytes_t,
}

