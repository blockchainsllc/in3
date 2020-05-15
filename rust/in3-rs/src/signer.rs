use async_trait::async_trait;
use ffi::{CStr, CString};
use libc::{c_char, strlen};
use serde_json::json;
use std::ffi;
use std::str;

use crate::traits::Signer;
use rustc_hex::FromHex;
use secp256k1::{sign, Message, SecretKey};
use sha3::{Digest, Keccak256Full};
use std::fmt::Write;

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum SignatureType {
    Raw = 0,
    Hash = 1,
}
pub unsafe fn signc(pk: *mut u8, data: *const c_char, len: usize) -> *mut u8 {
    let data_ = data as *mut u8;
    let dst: *mut u8 = libc::malloc(65) as *mut u8;
    let pby = dst.offset(64) as *mut u8;
    let error = in3_sys::sign_hash(data_, len as u8, pk, in3_sys::hasher_t::hasher_sha3k, dst);
    if error < 0 {
        panic!("Sign error{:?}", error);
    }
    *dst.offset(64) += 27;
    dst
}

fn signature_hex_string(data: [u8; 64]) -> String {
    let mut sign_str = "".to_string();
    for byte in &data[0..64] {
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", byte).unwrap();
        sign_str.push_str(tmp.as_str());
    }
    sign_str
}

pub struct SignerRust<'a> {
    pub pk: &'a str,
}

impl Signer for SignerRust<'_> {
    fn sign(&mut self, msg: &str) -> Option<String> {
        let msg_hex = msg.from_hex().unwrap();
        let pk_hex = self.pk.from_hex().unwrap();
        let mut hasher = Keccak256Full::new();
        // write input message
        hasher.input(msg_hex);
        // read hash digest
        let result = hasher.result();
        let mut msg_slice: [u8; 32] = Default::default();
        msg_slice.copy_from_slice(&result[0..32]);
        let mut pk_slice: [u8; 32] = Default::default();
        pk_slice.copy_from_slice(&pk_hex[0..32]);
        let seckey = SecretKey::parse(&pk_slice).unwrap();
        let message = Message::parse(&msg_slice);
        let (signature, _) = sign(&message, &seckey);
        let mut signature_arr = signature.serialize();
        // signature_arr[63] += 27;
        let sign_str = signature_hex_string(signature_arr);
        Some(sign_str)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_signature() {
        let msg = "9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465";
        let pk = "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
        let msg_hex = msg.from_hex().unwrap();
        let pk_hex = pk.from_hex().unwrap();
        let mut hasher = Keccak256Full::new();
        // write input message
        hasher.input(msg_hex);
        // read hash digest
        let result = hasher.result();
        let mut msg_slice: [u8; 32] = Default::default();
        msg_slice.copy_from_slice(&result[0..32]);
        let mut pk_slice: [u8; 32] = Default::default();
        pk_slice.copy_from_slice(&pk_hex[0..32]);
        let seckey = SecretKey::parse(&pk_slice).unwrap();
        let message = Message::parse(&msg_slice);
        let (signature, _) = sign(&message, &seckey);
        let signature_arr = signature.serialize();
        let sign_str = signature_hex_string(signature_arr);
        println!(" signature {}", sign_str);
    }
}
