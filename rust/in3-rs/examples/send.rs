extern crate in3;

use futures::executor::block_on;

use in3::prelude::*;

async fn send_async() {
    let mut client = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let mut ctx = Ctx::new(&mut client, r#"{"method": "eth_blockNumber", "params": []}"#);
    client.send(&mut ctx);
}

fn send_request() {
    let mut in3 = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let mut ctx = Ctx::new(&mut in3, r#"{"method": "eth_blockNumber", "params": []}"#);
    let _res = in3.execute(&mut ctx);
    println!("end");
}

fn rpc_call() {
    let mut client = Client::new(chain::MAINNET);
    client.configure(r#"{"autoUpdateList":false,"nodes":{"0x7d0": {"needsUpdate":false}}}"#);
    match client.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}

fn main() {
    rpc_call();
    let future = send_async();
    block_on(future);
    send_request();
}



