extern crate in3;
use async_std::task;
use in3::prelude::*;
use std::{fmt::Write, num::ParseIntError};
use std::ffi::{CStr, CString};
use libc::{c_char, puts, strlen};
pub fn decode_hex(s: &str) -> Result<Vec<u8>, ParseIntError> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16))
        .collect()
}

fn decode_test(){
    let mut in3 = Client::new(chain::MAINNET);
    let pk_ = decode_hex("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8").unwrap();
    let pk_rust = pk_.as_ptr();
    let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
    let data_rust = data_.as_ptr();
    let pk_c = in3.hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
    let data_c = in3.new_bytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    //TODO: maybe assert this byte by byte
    // assert!(pk_c, pk_);
    // assert!(data_c, data_);
}

fn sign_sha() {
     unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        // let data = in3.new_bytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        // let c_data = data as *const c_char;
        // println!("{:?}--->", data_.len());
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        let signa = ctx.sign(0, c_data, data_.len());
        println!("SHA {:?}",signa);
     }
}

 fn sign_raw() {
     unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        // let src_str = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        // println!("{:?}", src_str.len());
        // let data = in3.new_bytes(src_str);
        // // let c_str_data = CString::new(src_str).unwrap(); // from a &str, creates a new allocation
        // // let c_data: *const c_char = c_str_data.as_ptr();
        // let c_data = data as *const c_char;
        let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        // println!("{:?}", data_.len());
        let signa = ctx.sign(1, c_data, data_.len());
        println!(" RAW > {:?}",signa);
     }
}

fn sign_execute() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    c.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
    // let request = r#"{"method": "in3_signData",   "params": ["0x0102030405060708090a0b0c0d0e0f","0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852","raw"] }"#;
    // let request = r#"{"method":"eth_sendTransaction", "params":[{ "gas": "0x76c0","nonce": "0x15","gasPrice": "0x9184e72a000","from": "0xb60e8dd61c5d32be8058bb8eb970870f07233155", "to":"0x45d45e6ff99e6c34a235d263965910298985fcfe", "value":"0xff" }]}"#;
    // let request = r#"{"method":"eth_sendTransaction", "params":[{ "gas": "0x76c0","nonce": "0x15","gasPrice": "0x9184e72a000","from": "0xb60e8dd61c5d32be8058bb8eb970870f07233155", "to":"0x45d45e6ff99e6c34a235d263965910298985fcfe", "value":"0xff" }]}"#;
    // let request = r#"{"method":"eth_sendTransaction", "params":[{"from": "0xb60e8dd61c5d32be8058bb8eb970870f07233155", "to":"0x45d45e6ff99e6c34a235d263965910298985fcfe", "value":"0xff" }]}"#;
    // let request = r#"{"method": "eth_blockNumber", "params": []}"#;

        let request = r#"{"method":"eth_sendTransaction", "params":[{ "gas": "0x76c0","nonce": "0x15","gasPrice": "0x9184e72a000","from": "0xb60e8dd61c5d32be8058bb8eb970870f07233155", "to":"0x45d45e6ff99e6c34a235d263965910298985fcfe", "value":"0xff" }]}"#;
    // let request = r#"{"method":"eth_sendRawTransaction","params":["0xf86580850306dc4200830186a09445d45e6ff99e6c34a235d263965910298985fcfe808026a081406926bb6b08a2af307272c91ede1f1a35983183eefc1116061c0b03ad25b7a02d844da68d4f4ac8751918c09e3a00248bd572ebd1c12e651edc293b39bc34af"]}"#;
    match task::block_on(c.rpc(request)) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("Failed with error: {}", err),
    }
}

fn main() {
    sign_sha();
    sign_raw();
    sign_execute();
}
