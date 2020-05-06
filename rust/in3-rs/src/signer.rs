use async_trait::async_trait;
use ffi::{CStr, CString};
use in3_sys::ecdsa_sign;
use in3_sys::ecdsa_sign_digest;
use libc::{c_char, strlen};
use rustc_hex::{FromHex, ToHex};
use serde_json::json;
use std::ffi;
use std::str;

// use secp256k1::{sign, Message, SecretKey};
// use secp256k1_test::Secp256k1;

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum SignatureType {
    Raw = 0,
    Hash = 1,
}
pub unsafe fn sign(pk: *mut u8, type_: SignatureType, data: *const c_char, len: usize) -> *mut u8 {
    let data_ = data as *mut u8;
    let dst: *mut u8 = libc::malloc(65) as *mut u8;
    let pby = dst.offset(64) as *mut u8;
    let curve = in3_sys::secp256k1;
    match type_ {
        SignatureType::Raw => {
            let error = in3_sys::ecdsa_sign_digest(&curve, pk, data_, dst, pby, None);
            if error < 0 {
                panic!("Sign error{:?}", error);
            }
        }
        SignatureType::Hash => {
            let error = in3_sys::ecdsa_sign(
                &curve,
                in3_sys::HasherType::HASHER_SHA3K,
                pk,
                data_,
                len as u32,
                dst,
                pby,
                None,
            );
            if error < 0 {
                panic!("Sign error{:?}", error);
            }
        }
    }
    *dst.offset(64) += 27;
    dst
}
