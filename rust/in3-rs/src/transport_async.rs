extern crate reqwest;
extern crate serde;

use serde_json::Value;
use serde::{Deserialize, Serialize};



#[tokio::main]
async fn http_async(url: &str, payload: &str) -> Result<String, reqwest::Error> {
    // let v: Value = serde_json::from_str(payload).unwrap();
    //  let res: Value = reqwest::Client::new()
    //         .post(url)
    //         .json(&v)
    //         .send()
    //         .await?
    //         .json()
    //         .await?;
    // Ok(res.to_string())
    let v: Value = serde_json::from_str(payload).unwrap();
     let res: String = reqwest::Client::new()
            .post(url)
            .body(payload.to_string())
            .send()
            .await?
            .text()
            .await?;
    Ok(res.to_string())
}

pub(crate) fn transport_http(payload: &str, urls: &[&str]) -> Vec<Result<String, String>> {
    let mut responses = vec![];
    for url in urls {
        let res = http_async(url, payload);
        match  res {
            Err(_) => responses.push(Err("Transport error".to_string())),
            Ok(res) => responses.push(Ok(res))
        }
    }
    responses
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_transport_http_async() {
        let res = transport_http(r#"{"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"in3":{"verification":"proof","version": "2.1.0"}}"#
        , &["https://in3-v2.slock.it/mainnet/nd-3"]);
        println!("----- >{:?}", res);
    }
}