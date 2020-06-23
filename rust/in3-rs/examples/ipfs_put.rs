use serde::{Deserialize, Serialize};

use async_std::task;
use serde_json;

use in3::prelude::*;

//Define a struct to collect the parsed JSON response into. The struct needs an extra macro definition
//of serialize and desserialize as per the requirements of serde JSON
#[derive(Serialize, Deserialize)]
struct Response{
    result: String
}

fn main() {
    let mut c = Client::new(chain::IPFS);

    //c.rpc is an asynchrous request (due to the internal C library). Therefore to block execution
    //we use async_std's block_on function
    match task::block_on(c.rpc(r#"{"method":"ipfs_put","params":["I love Incubed","utf8"]}"#)) {
        Ok(res) => {
            //the parsed output is a vector of responses because the JSON response from in3
            //is packed inside an array i.e. the JSON string looks something like "[{JSON object}]"
            let v: Vec<Response> = serde_json::from_str(&res).unwrap();

            //print out the IPFS hash
            println!("The hash is {:?}", v[0].result)
        },
        Err(err) => println!("Failed with error: {}", err),
    }
}