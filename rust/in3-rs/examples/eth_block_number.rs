extern crate in3;

use in3::prelude::*;

fn main() {
    let mut c = Client::new(ChainId::Mainnet);
    c.set_transport(Box::new(|payload: &str, urls: &[&str]| {
        let mut responses = vec![];
        responses.push(Ok("{kjlk}".to_string()));
        responses.push(Err("{asd}".to_string()));
        responses
    }));
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}