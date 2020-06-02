use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::error::*;
use crate::traits::Client;

#[derive(Serialize)]
pub struct Request<'a> {
    pub method: &'a str,
    pub params: Value,
}

#[derive(Debug, Deserialize, Clone)]
pub struct Response {
    pub result: Option<Value>,
    pub error: Option<Value>,
}

impl From<Response> for In3Result<Value> {
    fn from(r: Response) -> Self {
        if let Some(res) = r.result {
            Ok(res)
        } else {
            Err(Error::CustomError(format!("{}", r.error.unwrap())))
        }
    }
}

impl Response {
    pub fn into_result(self) -> In3Result<Value> {
        self.into()
    }
}

pub async fn rpc(client: &mut Box<dyn Client>, params: Request<'_>) -> In3Result<Vec<Response>> {
    let req_str = serde_json::to_string(&params)?;
    let resp_str = client.rpc(req_str.as_str()).await?;
    let resp: Vec<Response> = serde_json::from_str(resp_str.as_str())?;
    Ok(resp)
}
