extern crate surf;

async fn http_async(
    url: &str,
    payload: &str,
) -> Result<String, Box<dyn std::error::Error + Send + Sync + 'static>> {
    let res: String = surf::post(url)
        .body_string(payload.to_string())
        .recv_string()
        .await?;
    Ok(res.to_string())
}

pub(crate) async fn transport_http(payload: &str, urls: &[&str]) -> Vec<Result<String, String>> {
    let mut responses = vec![];
    for url in urls {
        let res = http_async(url, payload).await;
        match res {
            Err(_) => responses.push(Err("Transport error".to_string())),
            Ok(res) => responses.push(Ok(res)),
        }
    }
    responses
}

#[cfg(test)]
mod tests {
    use super::*;
    use async_std::task;

    #[test]
    fn test_transport_http_async() {
        let res = task::block_on(transport_http(
            r#"{"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"in3":{"verification":"proof","version": "2.1.0"}}"#,
            &["https://in3-v2.slock.it/mainnet/nd-3"],
        ));
        println!("----- >{:?}", res);
    }
}
