extern crate reqwest;

use std::collections::HashMap;

fn http_send(url: &str, payload: &str) -> Result<String, Box<dyn std::error::Error>> {
    let client = reqwest::blocking::Client::new();
    let res = client.post(url)
        .body(payload.to_string())
        .send()?;
    Ok(res.text().unwrap())
}

fn transport_http(payload: &str, urls: &[&str]) -> Vec<Result<String, String>> {
    let mut responses = vec![];
    for url in urls {
        match http_send(url, payload) {
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
    fn test_http() {
        let res = transport_http(r#"{"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"in3":{"verification":"proof","version": "2.1.0"}}"#
                                 , &["https://in3-v2.slock.it/mainnet/nd-3"]);
        println!("{:?}", res);

    }
}