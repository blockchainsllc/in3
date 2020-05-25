//! Low-level, unsafe Rust bindings for the [`In3`][in3] library.
//!
//!
//! We recommend against using this crate directly.
//! Instead, consider using [in3-rs], which provides a high-level, safe, "Rusty" interface.
//!
//! [in3-rs]: https://github.com/in3-rust/in3-rs
//!
//! [in3 headers]: https://github.com/in3-rust/in3-sys/blob/master/in3/include/in3.h

// Suppress errors from In3 names
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(improper_ctypes)]
#![no_std]

extern crate libc;

// Bindings should be copied here
include!(concat!(env!("OUT_DIR"), "/in3.rs"));
