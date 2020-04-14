use async_trait::async_trait;

use crate::error;

#[async_trait]
pub trait Transport: Send {
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>>;
    fn fetch_blocking(&mut self, _request: &str, _uris: &[&str]) -> Vec<Result<String, String>> { vec![] }
}

pub trait Storage: Send {
    fn get(&self, key: &str) -> Vec<u8>;
    fn set(&mut self, key: &str, value: &[u8]);
    fn clear(&mut self);
}

#[async_trait]
pub trait Client: Send {
    fn box_new(transport: Box<dyn Transport>, storage: Box<dyn Storage>) -> Box<Self> where Self: Sized;
    // fn box_default() -> Box<Self> where Self: Sized;
    fn configure(&mut self, config: &str) -> Result<(), String>;
    fn version(&self) -> String;
    async fn rpc(&mut self, call: &str) -> error::In3Result<String>;
}

pub trait Api {
    fn new(client: Box<dyn Client>) -> Self;
    fn client(&mut self) -> &mut Box<dyn Client>;
}
