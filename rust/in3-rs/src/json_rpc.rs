//! Minimal JSON RPC implementation.
use std::convert;

use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::error;
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
#[derive(Debug, Deserialize, Serialize)]
pub struct Response {
    /// Result - required if `error` is `None`
    pub result: Option<Value>,
    /// Error - required if `result` is `None`
    pub error: Option<Value>,
}

/// JSON RPC error.
#[derive(Debug)]
pub enum Error {
    /// Neither `result` nor `error` field was set in response
    InvalidResponse(String),
    /// `error` field was set in response
    ErrorResponse(String),
}

impl convert::From<Error> for error::Error {
    fn from(err: Error) -> Self {
        Self::JsonRpcError(err)
    }
}

impl Response {
    /// Returns the [`Response`](struct.Response.html) interpreted a `Result` type
    pub fn to_result(&self) -> error::In3Result<&Value> {
        if let Some(ref res) = self.result {
            Ok(res)
        } else if let Some(ref err) = self.error {
            Err(Error::ErrorResponse(serde_json::to_string(err).unwrap()).into())
        } else {
            Err(Error::InvalidResponse(serde_json::to_string(self).unwrap()).into())
        }
    }
}

/// Generic function that performs the specified JSON RPC call and returns the result
/// deserialized as specified/inferred type
///
/// # Arguments
/// * `client` - reference to [`Client`](../in3/struct.Client.html) instance.
/// * `request` - request to perform.
pub async fn rpc<T>(client: &mut Box<dyn Client>, request: Request<'_>) -> error::In3Result<T>
where
    T: serde::de::DeserializeOwned,
{
    let req_str = serde_json::to_string(&request)?;
    let resp_str = client.rpc(req_str.as_str()).await?;

    //Check for array in or object in the response.
    let resp_: Vec<Response> = match serde_json::from_str(resp_str.as_str()) {
        Result::Ok(val) => val,
        Result::Err(_) => {
            let response = Response {
                result: Some(serde_json::Value::Null),
                error: Some(serde_json::Value::Null),
            };
            vec![response]
        }
    };

    //Check array is valid and try once again
    if resp_[0].result == Some(serde_json::Value::Null) {
        let resp_single: Response =
            serde_json::from_str(resp_str.as_str()).expect("response is not valid JSON-RPC result");
        return Ok(serde_json::from_str(
            resp_single.to_result()?.to_string().as_str(),
        )?);
    } else {
        let resp = resp_.first().expect("empty response");
        return Ok(serde_json::from_str(
            resp.to_result()?.to_string().as_str(),
        )?);
    }
}
