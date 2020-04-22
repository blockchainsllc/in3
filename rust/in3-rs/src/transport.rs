extern crate surf;

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

pub struct HttpTransport;

#[async_trait]
impl Transport for HttpTransport {
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>> {
        let mut responses = vec![];
        for url in uris {
            println!("{:?} {:?}", url, request);

            let res = http_async(url, request).await;
            println!("{:?}", res);
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
