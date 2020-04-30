// extern crate abi;
// extern crate in3;
use async_std::task;
use async_trait::async_trait;
use ethereum_types::{Address, U256};
use ffi::{CStr, CString};
use in3::eth1::api::RpcRequest;
use in3::eth1::*;
use in3::prelude::*;
use in3::signer;
use in3::signer::SignatureType;
use libc::c_char;
use rustc_hex::{FromHex, ToHex};
use serde_json::json;
use std::collections::HashMap;
use std::ffi;
use std::fmt::Write;
use std::num::ParseIntError;
use std::str;
// use crate::transport::MockTransport;

unsafe fn signature_hex_string(data: *mut u8) -> String {
    let value = std::slice::from_raw_parts_mut(data, 65 as usize);
    let mut sign_str = "".to_string();
    for byte in value {
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", byte).unwrap();
        sign_str.push_str(tmp.as_str());
    }
    println!(" signature {}", sign_str);
    sign_str
}

fn sign() {
    unsafe {
        //Private key
        let pk = "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
        //Message to sign
        let msg = "9fa034abf05bd334e60d92da257eb3d66dd3767bba9a1d7a7575533eb0977465";
        //Config in3 api client
        // decode Hex msg
        let msg_hex = msg.from_hex().unwrap();
        let raw_msg_ptr = msg_hex.as_ptr() as *const c_char;
        // pk to raw ptr
        let pk_hex = pk.from_hex().unwrap();
        let raw_pk = pk_hex.as_ptr() as *mut u8;
        //Sign the message raw
        let signature_raw = signer::sign(raw_pk, SignatureType::Raw, raw_msg_ptr, msg_hex.len());
        let sig_raw_expected = "f596af3336ac65b01ff4b9c632bc8af8043f8c11ae4de626c74d834412cb5a234783c14807e20a9e665b3118dec54838bd78488307d9175dd1ff13eeb67e05941c";
        assert_eq!(signature_hex_string(signature_raw), sig_raw_expected);
        // Hash and sign the msg
        let signature_hash = signer::sign(raw_pk, SignatureType::Hash, raw_msg_ptr, msg_hex.len());
        let sig_hash_expected = "349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c";
        assert_eq!(signature_hex_string(signature_hash), sig_hash_expected);
    }
}

fn sign_tx_api() {
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    let responses = vec![
        (
            "eth_estimateGas",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x96c0"}]"#,
        ),
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
        (
            "eth_gasPrice",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x9184e72a000"}]"#,
        ),
        (
            "eth_getTransactionCount",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        ),
    ];
    eth_api.client().configure(
        r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    eth_api
        .client()
        .set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    eth_api.client().set_transport(Box::new(MockTransport {
        responses: responses,
    }));
    let mut abi = abi::In3EthAbi::new();
    let params = task::block_on(abi.encode(
        "setData(uint256,string)",
        serde_json::json!([123, "testdata"]),
    ))
    .unwrap();
    println!("{:?}", params);
    let to: Address =
        serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap();
    let from: Address =
        serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap();
    let txn = OutgoingTransaction {
        to: to,
        from: from,
        data: Some(params),
        ..Default::default()
    };

    let hash: Hash = task::block_on(eth_api.send_transaction(txn)).unwrap();
    println!("Hash => {:?}", hash);
}

fn sign_tx_rpc() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(
        r#"{"proof":"none","autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    let responses = vec![
        (
            "eth_estimateGas",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#,
        ),
        (
            "eth_getTransactionCount",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        ),
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
    ];
    c.set_transport(Box::new(MockTransport {
        responses: responses,
    }));
    c.set_pk_signer("0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f");
    let tx = json!([{
        "from": "0x63FaC9201494f0bd17B9892B9fae4d52fe3BD377",
        "to": "0xd46e8dd67c5d32be8058bb8eb970870f07244567",
        "gas": "0x96c0",
        "gasPrice": "0x9184e72a000",
        "value": "0x9184e72a",
        "nonce": "0x0",
        "data": "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675"
    }]);
    let rpc_req = RpcRequest {
        method: "eth_sendTransaction",
        params: tx,
    };
    let req_str = serde_json::to_string(&rpc_req).unwrap();
    match task::block_on(c.rpc(&req_str)) {
        Ok(res) => println!("FINAL RPC RESPONSE > {:?}, {:?}\n\n", req_str, res),
        Err(err) => println!("Failed with error: {}\n\n", err),
    }
}

fn main() {
    sign_tx_api();
    sign_tx_rpc();
    sign();
}
