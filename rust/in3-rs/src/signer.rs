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
pub unsafe fn signc(pk: *mut u8, data: *const c_char, len: usize) -> Bytes {
    let data_ = data as *mut u8;
    let mut dst = [0u8; 65];
    let error = in3_sys::ec_sign_pk_hash(
        data_,
        len,
        pk,
        in3_sys::hasher_t::hasher_sha3k,
        dst.as_mut_ptr(),
    );
    if error < 0 {
        panic!("Sign error{:?}", error);
    }
    dst[64] += 27;
    dst[0..].into()
}

/// Signer implementation using IN3 C client's RPC.
pub struct In3Signer {
    in3: Box<Client>,
    pk: Bytes,
}

impl In3Signer {
    /// Create an In3Signer instance
    pub fn new(pk: Bytes) -> In3Signer {
        In3Signer {
            in3: Client::new(chain::LOCAL),
            pk,
        }
    }
}

#[async_trait(? Send)]
impl Signer for In3Signer {
    async fn sign(&mut self, msg: Bytes) -> In3Result<Bytes> {
        let resp_str = self
            .in3
            .rpc(
                serde_json::to_string(&json!({
                    "method": "in3_signData",
                    "params": [msg, self.pk]
                }))
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        let res: Bytes = serde_json::from_str(resp["result"]["signature"].to_string().as_str())?;
        Ok(res)
    }

    async fn prepare(&mut self, msg: Bytes) -> In3Result<Bytes> {
        let resp_str = self
            .in3
            .rpc(
                serde_json::to_string(&json!({
                    "method": "in3_prepareTx",
                    "params": [msg]
                }))
                .unwrap()
                .as_str(),
            )
            .await?;
        let resp: Value = serde_json::from_str(resp_str.as_str())?;
        let res: Bytes = serde_json::from_str(resp["result"].to_string().as_str())?;
        Ok(res)
    }
}

#[cfg(test)]
mod tests {
    use rustc_hex::FromHex;

    use super::*;

    #[test]
    fn test_signature() {
        let msg = "9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465"
            .from_hex()
            .unwrap();
        let pk = "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6"
            .from_hex()
            .unwrap();
        let mut s = In3Signer::new(pk.into());
        let signature = async_std::task::block_on(s.sign(msg.into()));
        let sign_str = format!("{:?}", signature.unwrap());
        assert_eq!(sign_str, "0x349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c");
    }
}
