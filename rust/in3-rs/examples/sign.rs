extern crate in3;
// extern crate abi;
use async_std::task;
use std::ffi;
use async_trait::async_trait;
use ethereum_types::{Address, U256};
use in3::eth1::api::RpcRequest;
use in3::eth1::*;
use in3::prelude::*;
use libc::c_char;
use serde_json::json;
use std::collections::HashMap;
use std::num::ParseIntError;
use std::str;
use ffi::{CStr, CString};
pub fn decode_hex(s: &str) -> Result<Vec<u8>, ParseIntError> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16))
        .collect()
}

struct MockTransport2<'a> {
    responses: Vec<(&'a str, &'a str)>,
}

#[async_trait]
impl Transport for MockTransport2<'_> {
    async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = self.responses.pop();
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        println!("\n\n Req: \n{:?}\n", request);
        println!("{:?}", response);
        match response {
            Some(response) if response.0 == request[0]["method"] => {
                println!(
                    "\n\nmethod {:?}, response > {:?}\n\n",
                    request[0]["method"],
                    response.1.to_string()
                );
                vec![Ok(response.1.to_string())]
            }
            _ => vec![Err(format!(
                "Found wrong/no response while expecting response for {}",
                request
            ))],
        }
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = self.responses.pop();
        let request: serde_json::Value = serde_json::from_str(_request).unwrap();
        println!("{:?}", request);
        println!("{:?}", request[0]["method"]);
        match response {
            Some(response) if response.0 == request[0]["method"] => {
                println!("--------- > {:?}", response.1.to_string());
                vec![Ok(response.1.to_string())]
            }
            _ => vec![Err(format!(
                "Found wrong/no response while expecting response for {}",
                request
            ))],
        }
    }
}

struct MockTransport<'a> {
    responses: HashMap<&'a str, &'a str>,
}

#[async_trait]
impl Transport for MockTransport<'_> {
    async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        let method_ = request.get(0).unwrap().get("method").unwrap().to_string();
        let method = serde_json::from_str::<String>(&method_).unwrap();

        let response = self.responses.get::<str>(&method.to_string());
        println!("{:?}, {:?} ", method, response);
        // let test= response.unwrap();
        // vec![Ok(String::from("{}"))]
        println!("{:?}", response);
        match response {
            Some(response) => {
                // let res_: serde_json::Value= serde_json::from_str(&response).unwrap();
                // let result_ =  res_.get(0).unwrap().get("result").unwrap().to_string();
                // let result = serde_json::from_str::<String>(&result_).unwrap();
                println!("{:?}", response.to_string());
                vec![Ok(response.to_string())]
            }
            _ => vec![Err(format!(
                "Found wrong/no response while expecting response for {}",
                request
            ))],
        }
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        unimplemented!()
    }
}

fn decode_test() {
    let mut in3 = Client::new(chain::MAINNET);
    let _pk =
        decode_hex("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8").unwrap();
    let _pk_rust = _pk.as_ptr();
    let _data =
        decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
    let _data_rust = _data.as_ptr();
    let _pk_c =
        in3.hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
    let _data_c = in3.new_bytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", 32);
    //TODO: maybe assert this byte by byte
    // assert!(pk_c, pk_);
    // assert!(data_c, data_);
}

fn sign_raw() {
    unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        let data_ =
            decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        let signa = ctx.sign(Signature::Raw, c_data, data_.len());
        println!("SHA {:?}", signa);
    }
}

fn sign_hash() {
    unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        let data_ =
            decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        // println!("{:?}", data_.len());
        let signa = ctx.sign(Signature::Hash, c_data, data_.len());
        println!(" RAW > {:?}", signa);
    }
}

fn sign() {
    unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        in3.set_pk_signer("0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f");
        let data_ =
            decode_hex("f852808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f07244567849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675018080").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        // println!("{:?}", data_.len());
        let signa = ctx.sign(Signature::Hash, c_data, data_.len());
        println!(" RAW > {:?}", signa);
        // assert_eq!(signa, "349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c");
    }
}

fn sign_params() {
    unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        // in3.set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
        in3.set_pk_signer("0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f");
        // let data_: String = "852808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f07244567849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f07244567501808".to_string();
        // let c_str_data = CString::new(data_.as_str()).unwrap(); // from a &str, creates a new allocation
        // let c_data: *const c_char = c_str_data.as_ptr();
        let hash = "f852808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f07244567849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675018080".to_string();
        let data_ = in3.new_bytes(&hash, 84);
        let c_data = data_ as *const c_char;
        // println!("{:?}", data_.len());
        let signa = ctx.sign(Signature::Hash, c_data, 168);
        println!(" RAW > {:?}", signa);
        // assert_eq!(signa, "349338b22f8c19d4c8d257595493450a88bb51cc0df48bb9b0077d1d86df3643513e0ab305ffc3d4f9a0f300d501d16556f9fb43efd1a224d6316012bb5effc71c");
    }
}

fn sign_execute_api() {
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    // let mut responses = HashMap::new();
    // responses.insert(r#""eth_gasPrice"#,r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    // responses.insert(r#""eth_estimateGas"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#);
    // responses.insert(r#"eth_getTransactionCount"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    // responses.insert(r#""eth_sendRawTransaction"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#);
    let responses = vec![
        (
            "eth_estimateGas",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#,
        ),
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
        (
            "eth_gasPrice",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
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
    eth_api.client().set_transport(Box::new(MockTransport2 {
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

fn sign_execute_arpc() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(
        r#"{"proof":"none","autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    let responses = vec![
        // (
        //     "eth_estimateGas",
        //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#,
        // ),
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
        // (
        //     "eth_getTransactionCount",
        //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        // ),
    ];
    c.set_transport(Box::new(MockTransport2 {
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
fn 
sign_execute_rpc() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(
        r#"{"proof":"none","autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#,
    );
    let responses = vec![
        (
            "eth_sendRawTransaction",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#,
        ),
        // (
        //     "eth_estimateGas",
        //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#,
        // ),
        // (
        //     "eth_gasPrice",
        //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        // ),
        // (
        //     "eth_getTransactionCount",
        //     r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#,
        // ),
    ];
    c.set_transport(Box::new(MockTransport2 {
        responses: responses,
    }));
    c.set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    let tx = json!([{
        "from": "0x63FaC9201494f0bd17B9892B9fae4d52fe3BD377",
        "to": "0xd46e8dd67c5d32be8058bb8eb970870f07244567",
        "gas": "0x96c0",
        "gasPrice": "0x9184e72a000",
        "value": "0x9184e72a",
        "nonce": "0x0",
        "data": "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675"
    }]);
    // let tx = json!([{
    //     "from": "0x3fEfF9E04aCD51062467C494b057923F771C9423",
    //     "to": "0x1234567890123456789012345678901234567890",
    //     "gas": "0x96c0",
    //     "gasPrice": "0x9184e72a000",
    //     "value": "0x9184e72a",
    //     "data": "0x18562dae000000000000000000000000000000000000000000000000000000000000007b000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000087465737464617461000000000000000000000000000000000000000000000000"
    // }]);
    let rpc_req = RpcRequest {
        method: "eth_sendTransaction",
        params: tx,
    };
    let req_str = serde_json::to_string(&rpc_req).unwrap();
    match c.rpc_blocking(&req_str) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("Failed with error: {}", err),
    }
}

fn test_transport() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    c.set_transport(Box::new(MockTransport2 {
        responses: vec![(
            "eth_blockNumber",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
        )],
    }));
    match c.rpc_blocking(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("Failed with error: {}", err),
    }
}

fn main() {
    //  sign_hash();
    // sign_raw();
    // sign_execute_api();
    // sign_params();
    sign_execute_arpc();
    // sign_execute_rpc();
    // test_transport();
    sign();
}
