use alloc::string::{String, ToString};
use core::convert::From;
use core::marker::PhantomData;
use core::mem;


use in3_sys::bytes_t;

#[derive(Debug)]
pub struct In3 {
    detail_enabled: bytes_t,

}

