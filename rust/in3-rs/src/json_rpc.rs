use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::error::*;
use crate::traits::Client;

#[derive(Serialize)]
pub struct Request<'a> {
    pub method: &'a str,
    pub params: Value,
}

#[derive(Debug, Deserialize)]
pub struct Response {
    pub result: Option<Value>,
    pub error: Option<Value>,
}

impl Response {
    pub fn to_result(&self) -> In3Result<&Value> {
        if let Some(ref res) = self.result {
            Ok(res)
        } else {
            Err(Error::CustomError(format!("{}", self.error.as_ref().unwrap())))
        }
    }
}

pub async fn rpc<T>(client: &mut Box<dyn Client>, request: Request<'_>) -> In3Result<T>
    where T: serde::de::DeserializeOwned {
    let req_str = serde_json::to_string(&request)?;
    let resp_str = client.rpc(req_str.as_str()).await?;
    let resp: Vec<Response> = serde_json::from_str(resp_str.as_str())?;
    let resp = resp.first().unwrap();
    Ok(serde_json::from_str(resp.to_result()?.to_string().as_str())?)
}
