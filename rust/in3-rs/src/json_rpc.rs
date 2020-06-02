use serde::Serialize;

use crate::error::*;
use crate::traits::Client;

#[derive(Serialize)]
pub struct Request<'a> {
    pub method: &'a str,
    pub params: serde_json::Value,
}

pub async fn rpc(client: &mut Box<dyn Client>, params: Request<'_>) -> In3Result<serde_json::Value> {
    let req_str = serde_json::to_string(&params)?;
    let resp_str = client.rpc(req_str.as_str()).await?;
    let resp: serde_json::Value = serde_json::from_str(resp_str.as_str())?;
    Ok(resp)
}
