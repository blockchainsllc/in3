//! Low-level, unsafe Rust bindings for the [`In3`][in3] disassembly library.
//!
//!
//! We recommend against using this crate directly.
//! Instead, consider using [in3-rs], which provides a high-level, safe, "Rusty" interface.
//!
//! [in3-rs]: https://github.com/in3-rust/in3-rs
//!
//! **Note**: documentation for functions/types was taken directly from
//! [In3 C headers][in3 headers].
//!
//! [in3 headers]: https://github.com/in3-rust/in3-sys/blob/master/in3/include/in3.h
//! <sup>1</sup>: Defined as a ["constified" enum modules](https://docs.rs/bindgen/0.30.0/bindgen/struct.Builder.html#method.constified_enum_module)
//!               because discriminant values are not unique. Rust requires discriminant values to be unique.

// Suppress errors from In3 names
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(improper_ctypes)]
#![no_std]

extern crate libc;

// Bindings should be copied here
include!(concat!(env!("CARGO_MANIFEST_DIR"), "/pre_generated/in3.rs"));

include!(concat!(env!("CARGO_MANIFEST_DIR"), "/common.rs"));
