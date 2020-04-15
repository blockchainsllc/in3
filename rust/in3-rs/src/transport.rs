extern crate surf;

use std::fs;

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
    Ok(res.to_string())
}

pub struct HttpTransport;

#[async_trait]
impl crate::traits::Transport for HttpTransport {
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>> {
        let mut responses = vec![];
        for url in uris {
            let res = http_async(url, request).await;
            match res {
                Err(_) => responses.push(Err("Transport error".to_string())),
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

pub struct FsStorage {
    pub dir: String
}

impl crate::traits::Storage for FsStorage {
    fn get(&self, key: &str) -> Option<Vec<u8>> {
        match fs::read(format!("{}/{}", self.dir, key)) {
            Ok(value) => Some(value),
            Err(_) => None,
        }
    }

    fn set(&mut self, key: &str, value: &[u8]) {
        fs::write(format!("{}/{}", self.dir, key), value).expect("Unable to write file");
    }

    fn clear(&mut self) {
        fs::remove_dir_all(format!("{}", self.dir)).unwrap();
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;

    use super::*;

    #[test]
    fn test_transport_http_async() {
        let mut transport = HttpTransport {};
        let res = task::block_on(transport.fetch(
            r#"{"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"in3":{"verification":"proof","version": "2.1.0"}}"#,
            &["https://in3-v2.slock.it/mainnet/nd-3"],
        ));
        println!("----- >{:?}", res);
    }

    #[test]
    fn test_transport_http_blocking() {
        let mut transport = HttpTransport {};
        let res = transport.fetch_blocking(
            r#"{"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"in3":{"verification":"proof","version": "2.1.0"}}"#,
            &["https://in3-v2.slock.it/mainnet/nd-3"],
        );
        println!("----- >{:?}", res);
    }
}
