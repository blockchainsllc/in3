//! Minimal JSON RPC implementation.
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::error::*;
use crate::traits::Client;

/// JSON RPC request.
#[derive(Serialize)]
pub struct Request<'a> {
    /// Method
    pub method: &'a str,
    /// Parameters (encoded as JSON array)
    pub params: Value,
}

/// JSON RPC response.
#[derive(Debug, Deserialize)]
pub struct Response {
    /// Result - required if `error` is `None`
    pub result: Option<Value>,
    /// Error - required if `result` is `None`
    pub error: Option<Value>,
}

impl Response {
    /// Returns the [`Response`](struct.Response.html) interpreted a `Result` type
    pub fn to_result(&self) -> In3Result<&Value> {
        if let Some(ref res) = self.result {
            Ok(res)
        } else {
            Err(Error::CustomError(format!("{}", self.error.as_ref().unwrap())))
        }
    }
}

/// Generic function that performs the specified JSON RPC call and returns the result
/// deserialized as specified/inferred type
///
/// # Arguments
/// * `client` - reference to [`Client`](../in3/struct.Client.html) instance.
/// * `request` - request to perform.
pub async fn rpc<T>(client: &mut Box<dyn Client>, request: Request<'_>) -> In3Result<T>
    where T: serde::de::DeserializeOwned {
    let req_str = serde_json::to_string(&request)?;
    let resp_str = client.rpc(req_str.as_str()).await?;
    let resp: Vec<Response> = serde_json::from_str(resp_str.as_str())?;
    let resp = resp.first().unwrap();
    Ok(serde_json::from_str(resp.to_result()?.to_string().as_str())?)
}