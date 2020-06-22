//! Signer trait implementations.
use libc::c_char;
use serde_json::{json, Value};

use async_trait::async_trait;

use crate::error::In3Result;
use crate::in3::{chain, Client};
use crate::traits::{Client as ClientTrait, Signer};
use crate::types::Bytes;

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
    dst
}

/// Signer implementation using IN3 C client's RPC.
pub struct In3Signer<'a> {
    in3: Box<Client>,
    pub pk: &'a str,
}

impl In3Signer<'_> {
    /// Create an In3Signer instance
    pub fn new(pk: &str) -> In3Signer {
        In3Signer {
            in3: Client::new(chain::LOCAL),
            pk,
        }
    }
}

#[async_trait(? Send)]
impl Signer for In3Signer<'_> {
    async fn sign(&mut self, msg: &str) -> In3Result<Bytes> {
        let resp_str = self
            .in3
            .rpc(
                serde_json::to_string(&json!({
                    "method": "in3_signData",
                    "params": [format!("0x{}", msg), format!("0x{}", self.pk)]
                })).unwrap().as_str()
            ).await?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        let res: Bytes = serde_json::from_str(resp["result"]["signature"].to_string().as_str())?;
        Ok(res)
    }
}

#[cfg(test)]
mod tests {
    use std::fmt::Write;

    use super::*;

    fn signature_hex_string(data: [u8; 64]) -> String {
        let mut sign_str = "".to_string();
        for byte in &data[0..64] {
            let mut tmp = "".to_string();
            write!(&mut tmp, "{:02x}", byte).unwrap(); // cannot fail
            sign_str.push_str(tmp.as_str());
        }
        //Equivalent to recoverycode ethereum += 27
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", 28).unwrap(); // cannot fail
        sign_str.push_str(tmp.as_str());
        sign_str
    }

    #[test]
    fn test_signature() {
        let msg = "9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465";
        let pk = "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
        let msg_hex = msg.from_hex().unwrap(); // cannot fail
        let pk_hex = pk.from_hex().unwrap(); // cannot fail
        let mut hasher = Keccak256Full::new();
        // write input message
        hasher.input(msg_hex);
        // read hash digest
        let result = hasher.result();
        let mut msg_slice: [u8; 32] = Default::default();
        msg_slice.copy_from_slice(&result[0..32]);
        let mut pk_slice: [u8; 32] = Default::default();
        pk_slice.copy_from_slice(&pk_hex[0..32]);
        let seckey = SecretKey::parse(&pk_slice).unwrap(); // cannot fail
        let message = Message::parse(&msg_slice);
        let (signature, _) = sign(&message, &seckey);
        let signature_arr = signature.serialize();
        let sign_str = signature_hex_string(signature_arr);
        println!(" signature {}", sign_str);
        assert_eq!(sign_str, "349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c");
    }
}
