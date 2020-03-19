//! Low-level, unsafe Rust bindings for the [`In3`][in3] disassembly library.
//!
//!
//! We recommend against using this crate directly.
//! Instead, consider using [in3-rs], which provides a high-level, safe, "Rusty" interface.
//!
//! [in3]: https://github.com/aquynh/in3
//! [in3-rs]: https://github.com/in3-rust/in3-rs
//!
//! # Supported disassembly architectures
//!
//! * `arm`: ARM
//! * `arm64`: ARM64 (also known as AArch64)
//! * `mips`: MIPS
//! * `ppc`: PowerPC
//! * `sparc`: SPARC
//! * `sysz`: System z
//! * `x86`: x86 family (includes 16, 32, and 64 bit modes)
//! * `xcore`: XCore
//!
//! For each architecture, *at least* the following types are defined (replace `ARCH` with
//! architecture names shown above):
//!
//! * `enum ARCH_insn`: instruction ids
//! * `enum ARCH_insn_group`: architecture-specific group ids
//! * `enum ARCH_op_type`: instruction operand types ids
//! * `enum ARCH_reg`<sup>1</sup>: register ids
//! * `struct ARCH_op_mem`: operand referring to memory
//! * `struct cs_ARCH_op`: instruction operand
//! * `struct cs_ARCH`: instruction
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

use libc::c_int;

// Bindings should be copied here
include!(concat!(env!("OUT_DIR"), "/in3.rs"));

include!(concat!(env!("CARGO_MANIFEST_DIR"), "/common.rs"));
