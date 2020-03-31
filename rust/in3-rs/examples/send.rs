extern crate in3;

use async_std::task;

use in3::prelude::*;

async fn send_async() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let mut ctx = Ctx::new(&mut c, r#"{"method": "eth_blockNumber", "params": []}"#);
    c.send(&mut ctx);
}

fn send_request() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    let mut ctx = Ctx::new(&mut c, r#"{"method": "eth_blockNumber", "params": []}"#);
    let _res = ctx.execute();
}

fn rpc_call() {
    let mut c = Client::new(chain::MAINNET);
    let _ = c.configure(r#"{"autoUpdateList":false,"nodes":{"0x1":{"needsUpdate":false}}}}"#);
    match c.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}

fn main() {
    rpc_call();
    let future = send_async();
    task::block_on(future);
    send_request();
}



