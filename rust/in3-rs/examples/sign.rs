use std::convert::TryInto;

use async_std::task;
use ethereum_types::Address;
use serde_json::json;

use in3::eth1::*;
use in3::json_rpc::Request;
use in3::prelude::*;

fn sign_tx_api() {
    // Config in3 api client
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
    let _ = eth_api.client().configure(
        r#"{"proof":"none", "autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    eth_api.client().set_signer(Box::new(In3Signer::new(
        "889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6"
            .try_into()
            .unwrap(),
    )));
    eth_api
        .client()
        .set_transport(Box::new(MockTransport { responses }));
    let mut abi = abi::In3EthAbi::new();
    let params = task::block_on(abi.encode(
        "setData(uint256,string)",
        serde_json::json!([123, "testdata"]),
    ))
    .expect("failed to ABI encode params");
    println!("{:?}", params);

    let to: Address =
        serde_json::from_str(r#""0x1234567890123456789012345678901234567890""#).unwrap(); // cannot fail
    let from: Address =
        serde_json::from_str(r#""0x3fEfF9E04aCD51062467C494b057923F771C9423""#).unwrap(); // cannot fail
    let txn = OutgoingTransaction {
        to,
        from,
        data: Some(params),
        ..Default::default()
    };

    let hash: Hash =
        task::block_on(eth_api.send_transaction(txn)).expect("ETH send transaction failed");
    println!("Hash => {:?}", hash);
}

fn sign_tx_rpc() {
    // Config in3 api client
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
    c.set_transport(Box::new(MockTransport { responses }));
    c.set_signer(Box::new(In3Signer::new(
        "8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f"
            .try_into()
            .unwrap(),
    )));
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
        Ok(res) => println!("RESPONSE > {:?}, {:?}\n\n", req_str, res),
        Err(err) => println!("Failed with error: {:?}\n\n", err),
    }
}

fn main() {
    sign_tx_api();
    sign_tx_rpc();
}
