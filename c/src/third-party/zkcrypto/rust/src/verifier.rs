use crate::decoder::{Decoder, STANDARD_ENCODING_LENGTH};
use bellman::{PrimeField, PrimeFieldRepr};
use franklin_crypto::alt_babyjubjub::{fs::Fs, fs::FsRepr, AltJubjubBn256};
use franklin_crypto::eddsa::Signature;
use franklin_crypto::jubjub::edwards::Point;
use franklin_crypto::jubjub::FixedGenerators;
use musig::verifier::MuSigVerifier;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct MusigBN256WasmVerifier;

#[wasm_bindgen]
impl MusigBN256WasmVerifier {
    #[wasm_bindgen]
    pub fn verify(
        message: &[u8],
        encoded_pubkeys: &[u8],
        encoded_signature: &[u8],
    ) -> Result<bool, JsValue> {
        let jubjub_params = AltJubjubBn256::new();
        let generator = FixedGenerators::SpendingKeyGenerator;
        let rescue_params =
            franklin_crypto::rescue::bn256::Bn256RescueParams::new_checked_2_into_1();

        let pubkeys = Decoder::decode_pubkey_list(encoded_pubkeys)?;

        let sig_r = Point::read(
            &encoded_signature[..STANDARD_ENCODING_LENGTH],
            &jubjub_params,
        )
        .unwrap();

        let mut repr = FsRepr::default();
        repr.read_be(&encoded_signature[STANDARD_ENCODING_LENGTH..])
            .unwrap();
        let sig_s = Fs::from_repr(repr).unwrap();

        let signature = Signature { r: sig_r, s: sig_s };

        let is_valid = MuSigVerifier::verify(
            message,
            &pubkeys,
            &signature,
            &jubjub_params,
            generator,
            &rescue_params,
        )
        .unwrap();

        Ok(is_valid)
    }
}
