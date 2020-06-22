use std::convert::TryInto;

use async_std::task;
use serde_json::json;

use in3::json_rpc::Request;
use in3::prelude::*;

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

    // Rust implementation of this can be found in signer.rs
    c.set_signer(Box::new(In3Signer::new(
        "8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f".try_into().unwrap())));

    // Enable to change for c implementation of the signer
    // c.set_pk_signer("8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f");

    c.set_transport(Box::new(MockTransport {
        responses,
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
    let rpc_req = Request {
        method: "eth_sendTransaction",
        params: tx,
    };
    let req_str = serde_json::to_string(&rpc_req).unwrap(); // Serialize `Request` impl cannot fail
    match task::block_on(c.rpc(&req_str)) {
        Ok(res) => println!("RESPONSE > {:?}, {:?}", req_str, res),
        Err(err) => println!("Failed with error: {}", err),
    }
}
