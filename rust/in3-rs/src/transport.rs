//! Transport trait implementations used by default or in tests.
use async_trait::async_trait;

use crate::traits::Transport;

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
        // println!("{:?}", request.to_string());

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
            let res = async_std::task::block_on(http_async(url, request));
            match res {
                Err(_) => responses.push(Err("Transport error".to_string())),
                Ok(res) => responses.push(Ok(res)),
            }
        }
        responses
    }
}
