mod tests {
    use crate::decoder::STANDARD_ENCODING_LENGTH;
    use crate::errors::MusigABIError;
    use crate::signer::MusigBN256WasmSigner;
    use crate::verifier::MusigBN256WasmVerifier;
    use bellman::pairing::bn256::Bn256;
    use bellman::{Field, PrimeField, PrimeFieldRepr};
    use franklin_crypto::alt_babyjubjub::fs::Fs;
    use franklin_crypto::alt_babyjubjub::AltJubjubBn256;
    use franklin_crypto::eddsa::{PrivateKey, PublicKey};
    use franklin_crypto::jubjub::FixedGenerators;
    use musig::errors::MusigError;
    use rand::{Rng, SeedableRng, XorShiftRng};
    use wasm_bindgen_test::*;

    use byteorder::{BigEndian, ByteOrder};

    fn musig_wasm_bn256_deterministic_setup(
        number_of_participants: usize,
        generator: FixedGenerators,
    ) -> Result<(Vec<PrivateKey<Bn256>>, Vec<PublicKey<Bn256>>), MusigError> {
        let jubjub_params = AltJubjubBn256::new();

        let mut privkeys = vec![];
        let mut pubkeys = vec![];

        let mut privkey = Fs::zero();
        for i in 0..number_of_participants {
            privkey.add_assign(&Fs::one());
            privkeys.push(PrivateKey::<Bn256>(privkey));
            pubkeys.push(PublicKey::from_private(
                &privkeys[i],
                generator,
                &jubjub_params,
            ));
        }

        Ok((privkeys, pubkeys))
    }

    fn musig_wasm_multiparty_full_round() {
        let number_of_parties = 2;

        let message = vec![1, 2, 3, 4, 5];

        let (privkeys, pubkeys) = musig_wasm_bn256_deterministic_setup(
            number_of_parties,
            FixedGenerators::SpendingKeyGenerator,
        )
        .unwrap();

        let pubkey_len = STANDARD_ENCODING_LENGTH;
        let sig_len = 2 * STANDARD_ENCODING_LENGTH;

        let mut encoded_pubkeys = vec![0u8; number_of_parties * pubkey_len];

        for (position, pubkey) in pubkeys.iter().enumerate() {
            let offset = position * pubkey_len;
            pubkey
                .write(&mut encoded_pubkeys[offset..(offset + pubkey_len)])
                .unwrap();
        }

        let mut wasm_signers = vec![];
        for position in 0..pubkeys.len() {
            let signer = MusigBN256WasmSigner::new(&encoded_pubkeys, position).unwrap();
            wasm_signers.push(signer);
        }
        assert!(wasm_signers.len() == number_of_parties);

        let rng = &mut XorShiftRng::from_seed([0x3dbe6259, 0x8d313d76, 0x3237db17, 0xe5bc0654]);

        let mut pre_commitments = vec![];
        for (position, wasm_signer) in wasm_signers.iter_mut().enumerate() {
            let mut encoded_privkey = vec![0u8; STANDARD_ENCODING_LENGTH];
            privkeys[position]
                .0
                .into_repr()
                .write_be(&mut encoded_privkey[..])
                .unwrap();

            // let pre_commitment =  wasm_signer.compute_precommitment(&encoded_privkey, &message).unwrap();
            let mut raw_seed = [0u8; 16];
            rng.fill_bytes(&mut raw_seed);

            let mut seed = [0u32; 4];
            seed[0] = BigEndian::read_u32(&raw_seed);
            seed[1] = BigEndian::read_u32(&raw_seed);
            seed[2] = BigEndian::read_u32(&raw_seed);
            seed[3] = BigEndian::read_u32(&raw_seed);

            let pre_commitment = wasm_signer.compute_precommitment(&seed).unwrap();
            pre_commitments.extend_from_slice(&pre_commitment);
        }
        assert!(pre_commitments.len() == number_of_parties * pubkey_len);

        let mut commitments = vec![];
        for wasm_signer in wasm_signers.iter_mut() {
            let commitment = wasm_signer
                .receive_precommitments(&pre_commitments)
                .unwrap();
            commitments.extend_from_slice(&commitment);
        }
        assert!(commitments.len() == number_of_parties * pubkey_len);

        let mut aggregated_commitments = vec![];
        for wasm_signer in wasm_signers.iter_mut() {
            let agg_commitment = wasm_signer.receive_commitments(&commitments).unwrap();
            aggregated_commitments.extend_from_slice(&agg_commitment);
        }

        assert!(aggregated_commitments.len() == number_of_parties * pubkey_len);
        let first_agg_commitment = aggregated_commitments[0..STANDARD_ENCODING_LENGTH].to_vec();
        for position in (0..pubkeys.len()).skip(1) {
            let offset = position * pubkey_len;
            assert_eq!(
                first_agg_commitment[..STANDARD_ENCODING_LENGTH],
                aggregated_commitments[offset..(offset + pubkey_len)]
            );
        }

        let mut signature_shares = vec![];
        for (position, wasm_signer) in wasm_signers.iter_mut().enumerate() {
            let mut encoded_privkey = vec![0u8; STANDARD_ENCODING_LENGTH];
            privkeys[position]
                .0
                .into_repr()
                .write_be(&mut encoded_privkey[..])
                .unwrap();
            let sig_share = wasm_signer.sign(&encoded_privkey, &message).unwrap();

            signature_shares.extend_from_slice(&sig_share);
        }
        assert!(signature_shares.len() == number_of_parties * pubkey_len);

        let mut aggregated_signatures = vec![];
        for wasm_signer in wasm_signers.iter_mut() {
            let agg_sig = wasm_signer
                .receive_signature_shares(&signature_shares)
                .unwrap();

            aggregated_signatures.extend_from_slice(&agg_sig);
        }

        assert!(aggregated_signatures.len() == number_of_parties * sig_len);

        let first_agg_sig = aggregated_signatures[..sig_len].to_vec();

        for position in (0..pubkeys.len()).skip(1) {
            let offset = position * sig_len;
            let sig = aggregated_signatures[(offset)..(offset + sig_len)].to_vec();
            assert_eq!(first_agg_sig[..sig_len], sig[..]);

            // verify aggregated signature
            let is_verified =
                MusigBN256WasmVerifier::verify(&message, &encoded_pubkeys, &sig).unwrap();
            assert!(is_verified);
        }
    }

    #[test]
    fn test_musig_wasm_multiparty_full_round() {
        musig_wasm_multiparty_full_round()
    }

    #[wasm_bindgen_test]
    fn test_musig_wasm() {
        musig_wasm_multiparty_full_round();
    }

    #[wasm_bindgen_test]
    fn test_invalid_pubkey_length() {
        let number_of_parties = 2;

        let (_, pubkeys) = musig_wasm_bn256_deterministic_setup(
            number_of_parties,
            FixedGenerators::SpendingKeyGenerator,
        )
        .unwrap();

        let pubkey_len = STANDARD_ENCODING_LENGTH;

        let mut encoded_pubkeys = vec![0u8; number_of_parties * pubkey_len];

        for (position, pubkey) in pubkeys.iter().enumerate() {
            let offset = position * pubkey_len;
            pubkey
                .write(&mut encoded_pubkeys[offset..(offset + pubkey_len)])
                .unwrap();
        }

        encoded_pubkeys.remove(1); // make pubkey list invalid

        for position in 0..pubkeys.len() {
            match MusigBN256WasmSigner::new(&encoded_pubkeys, position) {
                Err(e) => assert_eq!(e, MusigABIError::InvalidInputData.to_string()),
                _ => unreachable!(),
            }
        }
    }
}
