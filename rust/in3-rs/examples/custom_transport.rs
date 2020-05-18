extern crate in3;

use async_std::task;

use in3::prelude::*;

use in3::transport::MockTransport;

fn main() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    c.set_transport(Box::new(MockTransport {
        responses: vec![(
            "eth_blockNumber",
            r#"[{"jsonrpc":"2.0","id":1,"result":"0x96bacd"}]"#,
        )],
    }));
    match task::block_on(c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#)) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("Failed with error: {}", err),
    }
}
