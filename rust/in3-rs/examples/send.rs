use in3_sys::*;
use std::ffi;
use in3_sys::in3_ret_t;
extern crate in3;
use in3::prelude::*;
use futures::executor::block_on;

async fn send_async() {
    let mut client = Client::new(chain::MAINNET);
    client.set_auto_update_nodelist(false);
    let mut req_s = String::from(r#"{"method": "eth_blockNumber", "params": []}"#);
    let mut ctx = Ctx::new(&mut client, req_s);
    client.send(&mut ctx);
}

fn send_request() {
        let mut in3 = Client::new(chain::MAINNET);
        let mut req_s = String::from(r#"{"method": "eth_blockNumber", "params": []}"#);
        //let mut req_s = String::from("{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
        let mut ctx = Ctx::new(&mut in3, req_s);
        let res = in3.execute(&mut ctx);
        
        println!("end");
}
fn rpc_call(){
    let mut client = Client::new(chain::MAINNET);
    //let mut config = String::from("{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
    //client.configure(config);
    //client.set_auto_update_nodelist(false);
    match client.rpc(r#"{"method": "eth_blockNumber", "params": []}"#) {
        Ok(res) => println!("{}", res),
        Err(err) => println!("{}", err)
    }
}
fn main(){
   // rpc_call();
//    let future = send_async(); 
//    block_on(future);
    send_request();

}



