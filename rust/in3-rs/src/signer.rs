//! Signer trait implementations.
use std::str;

use libc::c_char;
use rustc_hex::FromHex;
use secp256k1::{Message, SecretKey, sign};
use sha3::{Digest, Keccak256Full};

use crate::traits::Signer;

/// Sign data using specified private key using low-level FFI types.
///
/// # Panics
/// This function does not report errors and panics instead.
///
/// # Safety
/// Being a thin wrapper over in3_sys::ec_sign_pk_hash(), this method is unsafe.
pub unsafe fn signc(pk: *mut u8, data: *const c_char, len: usize) -> *mut u8 {
    let data_ = data as *mut u8;
    let dst: *mut u8 = libc::malloc(65) as *mut u8;
    let error = in3_sys::ec_sign_pk_hash(data_, len, pk, in3_sys::hasher_t::hasher_sha3k, dst);
    if error < 0 {
        panic!("Sign error{:?}", error);
    }
    *dst.offset(64) += 27;
    dst
}


/// Signer implementation in pure and safe Rust.
pub struct SignerRust<'a> {
    pub pk: &'a str,
}

impl Signer for SignerRust<'_> {
    fn sign(&mut self, msg: &str) -> *const c_char {
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
        let signature_arr = signature.serialize();
        let ret_s = signature_arr.as_ptr();
        let ret_c_char = ret_s as *const c_char;
        ret_c_char
    }
}

#[cfg(test)]
mod tests {
    use std::fmt::Write;

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
        assert_eq!(sign_str, "349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c");
    }

    fn signature_hex_string(data: [u8; 64]) -> String {
        let mut sign_str = "".to_string();
        for byte in &data[0..64] {
            let mut tmp = "".to_string();
            write!(&mut tmp, "{:02x}", byte).unwrap();
            sign_str.push_str(tmp.as_str());
        }
        //Equivalent to recoverycode ethereum += 27
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", 28).unwrap();
        sign_str.push_str(tmp.as_str());
        sign_str
    }
}
