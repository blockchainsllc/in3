use async_trait::async_trait;

use crate::error;

#[async_trait]
pub trait Transport {
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>>;
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> { vec![] }
}

pub trait Storage {
    fn get(&self, key: &str) -> Option<Vec<u8>>;
    fn set(&mut self, key: &str, value: &[u8]);
    fn clear(&mut self);
}

#[async_trait(? Send)]
pub trait Client {
    fn configure(&mut self, config: &str) -> Result<(), String>;
    fn set_transport(&mut self, transport: Box<dyn Transport>);
    fn set_storage(&mut self, storage: Box<dyn Storage>);
    async fn rpc(&mut self, call: &str) -> error::In3Result<String>;
}

pub trait Api {
    fn new(client: Box<dyn Client>) -> Self;
    fn client(&mut self) -> &mut Box<dyn Client>;
}
