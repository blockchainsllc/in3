extern crate surf;

use async_trait::async_trait;

use crate::traits::Transport;
use std::error::Error;
use std::fs::File;
use std::io::BufReader;
use std::fmt::Write;
use std::path::{PathBuf, Path};
use std::env;

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

fn read_mock<P: AsRef<Path>>(path: P) -> Result<serde_json::Value, Box<Error>> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);
    let u = serde_json::from_reader(reader)?;
    Ok(u)
}
fn env_var(var: &str) -> String {
    env::var(var).expect(&format!("Environment variable {} is not set", var))
}

fn prepare_file_path(data:String) -> String{
    let mut relative_path = PathBuf::from(env_var("CARGO_MANIFEST_DIR"));
    relative_path.push("../../c/test/testdata/mock/");
    let mut full_path = relative_path.to_str().unwrap().to_string();
    let tmp = format!("{}.json", data);
    full_path.push_str(&tmp);
    println!("{:?}", full_path);
    full_path
}

fn read_json(data:String) -> String {
    let full_path = prepare_file_path(data);
    let value = read_mock(full_path).unwrap();
    let response = value["response"].to_string();
    response
}

pub struct MockJsonTransport<'a>{
    pub responses: &'a str,
}

#[async_trait]
impl Transport for MockJsonTransport<'_>  {

    async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = read_json(String::from(self.responses));
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        println!("--------> {:?}, \n\n {:?}", request.to_string(), response);
        vec![Ok(response)]
    }

    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        unimplemented!()
    }
}


pub struct MockTransport<'a> {
    pub responses: Vec<(&'a str, &'a str)>,
}

#[async_trait]
impl Transport for MockTransport<'_> {
    async fn fetch(&mut self, request: &str, _uris: &[&str]) -> Vec<Result<String, String>> {
        let response = self.responses.pop();
        let request: serde_json::Value = serde_json::from_str(request).unwrap();
        println!("{:?}", request.to_string());

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
