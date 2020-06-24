//! Transport trait implementations used by default or in tests.
use async_trait::async_trait;

use crate::traits::Transport;
use std::env;
use std::error::Error;
use std::fs;
use std::fs::File;
use std::io::BufReader;
use std::path::{Path, PathBuf};

async fn http_async(
    url: &str,
    payload: &str,
) -> Result<String, Box<dyn std::error::Error + Send + Sync + 'static>> {
    let res = surf::post(url)
        .body_string(payload.to_string())
        .set_header("content-type", "application/json")
        .recv_string()
        .await?;
    Ok(res)
}

/// Mock transport for use in json based test.
///
/// Read the contents of Mock response and request from a json file
///
/// See examples/custom_transport.rs for usage.

pub struct MockJsonTransport;
const MOCK_DIR_RELEASE: &'static str = "../../c/test/testdata/mock/";
const MOCK_DIR_DEBUG: &'static str = "../c/test/testdata/mock/";
impl MockJsonTransport {
    /// Read file from path
    ///
    /// Return serde:json Value or Error if file not found
    pub fn read_file<P: AsRef<Path>>(
        &mut self,
        path: P,
    ) -> Result<serde_json::Value, Box<dyn Error>> {
        let file = File::open(path)?;
        let reader = BufReader::new(file);
        let u = serde_json::from_reader(reader)?;
        Ok(u)
    }
    fn find_json_file(&mut self, name: String) -> Option<String> {
        let files = fs::read_dir(MOCK_DIR).unwrap();
        let json_files = files
            .filter_map(Result::ok)
            .filter(|d| d.path().extension().unwrap() == "json");
        for js in json_files {
            let file_name = js
                .path()
                .file_name()
                .unwrap()
                .to_os_string()
                .into_string()
                .unwrap();
            if file_name.starts_with(&name) {
                return Some(file_name);
            }
        }
        None
    }
    /// Helper for getting env vars
    pub fn env_var(&mut self, var: &str) -> String {
        env::var(var).expect(&format!("Environment variable {} is not set", var))
    }
    /// Get testdata path from in3c project
    pub fn prepare_file_path(&mut self, name: String) -> String {
        let mut relative_path = PathBuf::from(self.env_var("CARGO_MANIFEST_DIR"));
        relative_path.push(MOCK_DIR);
        let mut full_path = relative_path.to_str().unwrap().to_string();
        let data = self.find_json_file(name).unwrap();
        full_path.push_str(&data);
        full_path
    }
    /// Read and parse json from test data path
    pub fn read_json(&mut self, data: String) -> String {
        let full_path = self.prepare_file_path(data);
        let value = self.read_file(full_path).unwrap();
        let response = value["response"].to_string();
        response
    }
}

#[async_trait]
impl Transport for MockJsonTransport {
    /// Async fetch implementation
    ///
    /// Read responses from json
    async fn fetch(&mut self, request_: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let request: serde_json::Value = serde_json::from_str(request_).unwrap();
        let method_ = request[0]["method"].as_str();
        let response = self.read_json(String::from(method_.unwrap()));
        vec![Ok(response.to_string())]
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        unimplemented!()
    }
}

/// Mock transport for use in tests.
///
/// Maintains a vector of request-response string slice tuples which are treated like a FIFO stack.
/// This implementation gives IN3 the last pushed response on the stack if and only if the request
/// method matches the first element of the last pop'd tuple entry. Otherwise an error string is
/// returned.
///
/// See examples/custom_transport.rs for usage.
pub struct MockTransport<'a> {
    /// Vector of request-response string slice tuples.
    pub responses: Vec<(&'a str, &'a str)>,
}

/// Transport trait implementation for mocking in tests.
#[async_trait]
impl Transport for MockTransport<'_> {
    /// Async fetch implementation
    ///
    /// Pops the responses vector and returns it if it's associated request matches the i/p.
    /// Otherwise, returns an error string.
    async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = self.responses.pop();
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        match response {
            Some(response) if response.0 == request[0]["method"] => {
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
        unimplemented!()
    }
}

/// HTTP transport
///
/// This is the default transport implementation for IN3.
pub struct HttpTransport;

/// Transport trait implementation for HTTP transport.
#[async_trait]
impl Transport for HttpTransport {
    /// Fetches the responses from specified URLs over HTTP.
    /// Errors are reported as strings.
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>> {
        let mut responses = vec![];
        for url in uris {
            // println!("{:?} {:?}", url, request);
            let res = http_async(url, request).await;
            // println!("{:?}", res);
            match res {
                Err(err) => responses.push(Err(format!("Transport error: {:?}", err))),
                Ok(res) => responses.push(Ok(res)),
            }
        }
        responses
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>> {
        let mut responses = vec![];
        for url in uris {
            // println!("{:?} {:?}", url, request);
            let res = async_std::task::block_on(http_async(url, request));
            // println!("{:?}", res);
            match res {
                Err(_) => responses.push(Err("Transport error".to_string())),
                Ok(res) => responses.push(Ok(res)),
            }
        }
        responses
    }
}

#[cfg(test)]
mod tests {

    use crate::json_rpc::Response;
    use crate::prelude::*;
    use ethereum_types::U256;

    use super::*;
    #[test]
    fn test_json_tx_count() -> In3Result<()> {
        let mut transport = MockJsonTransport {};
        //Make use of static string literals conversion for mock transport.
        let method = String::from("eth_getTransactionCount");
        let response = transport.read_json(method).to_string();
        let resp: Vec<Response> = serde_json::from_str(&response)?;
        let result = resp.first().unwrap();
        let json_str = serde_json::from_str::<U256>(result.to_result()?.to_string().as_str())?;
        println!("{:?}", json_str);
        Ok(())
    }

    #[test]
    fn test_json_blk_by_hash() -> In3Result<()> {
        let mut transport = MockJsonTransport {};
        //Make use of static string literals conversion for mock transport.
        let method = String::from("eth_getBlockByHash");
        let response = transport.read_json(method).to_string();
        let resp: Vec<Response> = serde_json::from_str(&response)?;
        let result = resp.first().unwrap();
        let parsed = result.to_result()?;
        println!("{:?}", parsed["number"]);
        assert_eq!(parsed["number"], String::from("0x17a7a4"));
        Ok(())
    }
}
