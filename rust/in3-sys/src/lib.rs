//! Low-level, unsafe Rust bindings for the [`IN3`][https://github.com/slockit/in3-c/] library.
//!
//!
//! We recommend against using this crate directly.
//! Instead, consider using [in3-rs](https://github.com/in3-rust/in3-rs), which provides a high-level, safe and "Rusty" interface.
//!
//! **Note**: documentation for functions/types was taken directly from
//! [in3 headers](https://github.com/slockit/in3-c/blob/master/c/include/in3.rs.h)
//!

// Suppress errors from IN3 names
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(improper_ctypes)]
#![no_std]

extern crate libc;

// Bindings should be copied here
include!(concat!(env!("OUT_DIR"), "/in3.rs"));
