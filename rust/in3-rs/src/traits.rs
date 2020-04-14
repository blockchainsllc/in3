use async_trait::async_trait;

use crate::error;

#[async_trait]
pub trait Transport {
    async fn fetch(request: &str, uri: &[&str]) -> Vec<Result<String, String>>;
    fn fetch_blocking(_request: &str, _uri: &[&str]) -> Vec<Result<String, String>> { vec![] }
}

pub trait Storage {
    type DataStore;
    fn get(&self, key: &str) -> Vec<u8>;
    fn set(&mut self, key: &str, value: &[u8]);
    fn clear(&mut self);
}

#[async_trait]
pub trait Client: Transport + Storage {
    fn configure(&mut self, config: &str) -> Result<(), String>;
    fn version(&self) -> String;
    async fn rpc(&mut self, call: &str) -> error::In3Result<String>;
}

pub trait Api {
    type Client: Client;
    fn new(client: Self::Client) -> Self;
    fn client(&mut self) -> &mut Self::Client;
}
