extern crate in3;
extern crate hex;
use hex::FromHex;
use async_std::task;
use std::{fmt::Write, num::ParseIntError};
use in3::prelude::*;


pub fn decode_hex(s: &str) -> Result<Vec<u8>, ParseIntError> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16))
        .collect()
}

fn sign3() {
    let mut in3 = Client::new(chain::MAINNET);
    let pk_ = decode_hex("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8").unwrap();
    let pk = pk_.as_ptr();
    let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
    let data = data_.as_ptr();
    let signa = in3.eth_sign(1, pk,  data, 32);
    println!("\n");
}

fn sign2() {
    let mut in3 = Client::new(chain::MAINNET);
    let pk = in3.hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
    let data = in3.new_bytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    println!("{:?} {:?}",data, pk);
    let signa = in3.eth_sign(1, pk,  data, 32);
}

fn sign() {
    let mut in3 = Client::new(chain::MAINNET);
    let pk_ = hex::decode("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8").expect("-");
    let mut pk = pk_.as_ptr();
    let data_ = hex::decode("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
    let mut data= data_.as_ptr();
    let signa = in3.eth_sign(1, pk,  data, 32);
    // println!("{:?}", signa);
}

fn main() {
    
    sign();
    sign2();
    sign3();
}
