use franklin_crypto::alt_babyjubjub::AltJubjubBn256;
use musig::aggregated_pubkey::AggregatedPublicKey;
use crate::decoder::{Decoder, STANDARD_ENCODING_LENGTH};
use crate::errors::MusigABIError;
use wasm_bindgen::prelude::*;
extern crate core;

#[wasm_bindgen]
pub struct MusigBN256WasmAggregatedPubkey;

#[wasm_bindgen]
impl MusigBN256WasmAggregatedPubkey {
    #[wasm_bindgen]
    pub fn compute(
        encoded_pubkeys: &[u8],
    ) -> Result<Vec<u8>, JsValue> {
        let jubjub_params = AltJubjubBn256::new();

        let pubkeys = Decoder::decode_pubkey_list(encoded_pubkeys)?;       

        let (agg_pubkey, _ ) = AggregatedPublicKey::compute_for_each_party(&pubkeys, &jubjub_params).unwrap();
        
        let mut encoded_agg_pubkey = vec![0u8; STANDARD_ENCODING_LENGTH];
        
        agg_pubkey
            .write(&mut encoded_agg_pubkey[..])
            .map_err(|_| MusigABIError::EncodingError)?;

        Ok(encoded_agg_pubkey)
    }
}



#[no_mangle]
pub unsafe extern "C" fn zc_compute_aggregated_pubkey(pks: *mut u8, pks_len : usize, dst: *mut u8)  {
    std::ptr::copy(
        MusigBN256WasmAggregatedPubkey::compute(core::slice::from_raw_parts_mut(pks, pks_len))
        .unwrap().as_ptr(),
    dst, 32);
}
