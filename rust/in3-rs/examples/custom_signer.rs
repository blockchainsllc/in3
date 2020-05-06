
use std::fmt::Write;
use secp256k1::{sign, Message, SecretKey};
use rustc_hex::{FromHex};
use  sha3 :: { Digest , Keccak256Full };

// Public key for debug secret key
fn signature_hex_string(data: [u8; 64]) -> String {
    let mut sign_str= "".to_string();
    for byte in &data[0..64] {
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", byte).unwrap();
        sign_str.push_str(tmp.as_str());
    }
    sign_str
}

fn main() { 
    let msg = "9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465";
    let pk = "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
    let msg_hex = msg.from_hex().unwrap();
    let pk_hex = pk.from_hex().unwrap();
    let  mut  hasher  =  Keccak256Full :: new ();
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
    let (signature,_) = sign(&message, &seckey);
    let signature_arr = signature.serialize();
    let sign_str = signature_hex_string(signature_arr);
    println!(" signature {}", sign_str); 
}
