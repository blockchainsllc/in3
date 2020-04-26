extern crate in3;
// extern crate abi;
use async_std::task;
use in3::eth1::*;
use in3::eth1::api::{RpcRequest};
use in3::prelude::*;
use std::{num::ParseIntError};
use async_trait::async_trait;
use serde_json::json;
use ethereum_types::{Address, U256};
use libc::{c_char};
use std::str;
use std::collections::HashMap;
pub fn decode_hex(s: &str) -> Result<Vec<u8>, ParseIntError> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16))
        .collect()
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
        println!("{:?}, {:?} ",method, response);
        // let test= response.unwrap();
        // vec![Ok(String::from("{}"))]
        println!("{:?}",response);
        match  response {
            Some(response) => {
                // let res_: serde_json::Value= serde_json::from_str(&response).unwrap(); 
                // let result_ =  res_.get(0).unwrap().get("result").unwrap().to_string(); 
                // let result = serde_json::from_str::<String>(&result_).unwrap();
                println!("{:?}",response.to_string());
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

fn decode_test(){
    let mut in3 = Client::new(chain::MAINNET);
    let _pk = decode_hex("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8").unwrap();
    let _pk_rust = _pk.as_ptr();
    let _data = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
    let _data_rust = _data.as_ptr();
    let _pk_c = in3.hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
    let _data_c = in3.new_bytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    //TODO: maybe assert this byte by byte
    // assert!(pk_c, pk_);
    // assert!(data_c, data_);
}

fn sign_raw() {
     unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        let signa = ctx.sign(Signature::Raw, c_data, data_.len());
        println!("SHA {:?}",signa);
     }
}

 fn sign_hash() {
     unsafe {
        let mut in3 = Client::new(chain::MAINNET);
        let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
        in3.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
        let data_ = decode_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef").unwrap();
        let c_data = data_.as_ptr() as *const c_char;
        // println!("{:?}", data_.len());
        let signa = ctx.sign(Signature::Hash, c_data, data_.len());
        println!(" RAW > {:?}",signa);
     }
}


fn sign_execute_api() {
    let mut eth_api = Api::new(Client::new(chain::MAINNET));
    let mut responses = HashMap::new();
    responses.insert(r#""eth_gasPrice"#,r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    responses.insert(r#""eth_estimateGas"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#);
    responses.insert(r#"eth_getTransactionCount"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    responses.insert(r#""eth_sendRawTransaction"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#);
    eth_api
        .client()
        .configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    eth_api
        .client().set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    eth_api
        .client().set_transport(Box::new(MockTransport {
            responses: responses,
        }));
    
    let mut abi = abi::In3EthAbi::new();
    let mut params =
        task::block_on(abi.encode("test(address,string)", serde_json::json!(["0x1234567890123456789012345678901234567890", "xyz"]))).unwrap();
    println!(" PARAMS : {:?}", params);
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
    
    let hash: Hash = task::block_on(eth_api.send_transaction(txn))
        .unwrap();
    println!("Hash => {:?}", hash);
}


fn sign_execute_rpc() {
    
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let mut responses = HashMap::new();
    responses.insert(r#""eth_gasPrice"#,r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    responses.insert(r#""eth_estimateGas"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#);
    responses.insert(r#"eth_getTransactionCount"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#);
    responses.insert(r#""eth_sendRawTransaction"#, r#"[{"jsonrpc":"2.0","id":1,"result":"0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38"}]"#);
    c.set_transport(Box::new(MockTransport {
        responses: responses,
    })); 
    c.set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
    let tx = json!({
        "from": "0x3fEfF9E04aCD51062467C494b057923F771C9423",
        "to": "0x1234567890123456789012345678901234567890",
        "data": "0x18562dae000000000000000000000000000000000000000000000000000000000000007b000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000087465737464617461000000000000000000000000000000000000000000000000"
    });
    let rpc_req = RpcRequest {
        method: "eth_sendTransaction",
        params: tx,
    };
    let req_str = serde_json::to_string(&rpc_req).unwrap();
    println!("--- REQUEST :{:?}", req_str);
    match task::block_on(c.rpc(&req_str)){
        Ok(res) => println!("--- > {:?}, {:?}\n\n", req_str, res),
        Err(err) => println!("Failed with error: {}\n\n", err),
    }
}

// fn sign_execute_2() {
//     let tx = json!({
//         "from": "0x3fEfF9E04aCD51062467C494b057923F771C9423",
//         "to": "0x1234567890123456789012345678901234567890",
//         "data": params
//     });

//     let mut abi = abi::In3EthAbi::new();
//     let params =
//         task::block_on(abi.encode("setData(uint256,string)", serde_json::json!([123, "testdata"]))).unwrap();
//     println!("{:?}", params);
//     let mut c = Client::new(chain::MAINNET);
//     let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
//     // c.set_pk_signer("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8");
//     c.set_pk_signer("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
//     // let address = "0x3fEfF9E04aCD51062467C494b057923F771C9423";
//     // let request = r#"{"method":"eth_sendTransaction", 
//     // "params":[{ "gas": "0x76c0","nonce": "0x15","gasPrice": "0x9184e72a000",
//     // "from": "0x3fEfF9E04aCD51062467C494b057923F771C9423", 
//     // "to":"0x1234567890123456789012345678901234567890", 
//     // "data": params
//     // "value":"0x0" }]}"#;
//     let rpc_req = RpcRequest {
//         method: "eth_sendTransaction",
//         params: json!([filter_id]),
//     }
//     let responses = vec![("eth_gasPrice",r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"# ),
//     ("eth_estimateGas", r#"[{"jsonrpc":"2.0","id":1,"result":"0x1e8480"}]"#),
//     ("eth_getTransactionCount", r#"[{"jsonrpc":"2.0","id":1,"result":"0x0"}]"#),
//     ("eth_sendRawTransaction", "0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38")]; 
//     c.set_transport(Box::new(MockTransport {
//         responses: responses
//     })); 
//     match task::block_on(c.send(request)) {
//         Ok(res) => println!("--- > {}, {}", request, res),
//         Err(err) => println!("Failed with error: {}", err),
//     }
// }

fn main() {
    // sign_hash();
    // sign_raw();
    // sign_execute_api();
    sign_execute_rpc();
}
