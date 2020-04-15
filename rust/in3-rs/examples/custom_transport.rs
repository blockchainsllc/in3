extern crate in3;

use async_trait::async_trait;
use in3::prelude::*;

struct MockTransport<'a> {
    responses: Vec<(&'a str, &'a str)>
}

#[async_trait]
impl Transport for MockTransport<'_> {
    async fn fetch(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        unimplemented!()
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = self.responses.pop();
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        match response {
            Some(response) if response.0 == request[0]["method"] => vec![Ok(response.1.to_string())],
            _ => vec![Err(format!("Found wrong/no response while expecting response for {}", request))]
        }
    }
}

fn main() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    c.set_transport(Box::new(MockTransport {
        responses: vec![("eth_blockNumber", r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#)]
    }));
    match c.rpc_blocking(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err),
    }
}
