use async_std::task;
use ethereum_types::{Address, U256};
use in3::eth1::api::RpcRequest;
use in3::eth1::*;
use in3::prelude::*;
use in3::prelude::*;
use rustc_hex::FromHex;
use secp256k1::{sign, Message, SecretKey};
use serde_json::json;
use sha3::{Digest, Keccak256Full};
use std::ffi::{CStr, CString};
use std::fmt::Write;

// Public key for debug secret key
fn signature_hex_string(data: [u8; 64]) -> String {
    let mut sign_str = "".to_string();
    for byte in &data[0..64] {
        let mut tmp = "".to_string();
        write!(&mut tmp, "{:02x}", byte).unwrap();
        sign_str.push_str(tmp.as_str());
    }
    sign_str
}

fn main() {
    //Config in3 api client
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
    // c.set_signer(Box::new(SignerRust {
    //     pk: "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6",
    // }));
    c.set_pk_signer("889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    c.set_transport(Box::new(MockTransport {
        responses: responses,
    }));
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
        Ok(res) => println!("RESPONSE > {:?}, {:?}\n\n", req_str, res),
        Err(err) => println!("Failed with error: {}\n\n", err),
    }
}
