use in3_sys::*;
use std::ffi;
use in3_sys::in3_ret_t;
extern crate in3;
use in3::prelude::*;
use futures::executor::block_on;

async fn sendt() {
    let mut client = Client::new(ChainId::Mainnet);
    let mut config = String::from("{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
    client.configure(config);
    client.set_auto_update_nodelist(true);
    let mut req_s = String::from("{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
    let mut ctx = Ctx::new(&mut client, req_s);
    //let mut request = Request::new(&mut ctx);
    //let mut res = in3.execute(&mut ctx);
    client.send(&mut ctx);
    println!("end");
}

fn send_request() {
        let mut in3 = Client::new(ChainId::Mainnet);
        let mut req_s = String::from("{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
        //let mut req_s = String::from(r#"{"method":"eth_blockNumber","params":[]}"#);
        let mut ctx = Ctx::new(&mut in3, req_s);
        //let mut request = Request::new(&mut ctx);
        loop {
            let mut res = in3.execute(&mut ctx);
            match res {
                In3Ret::OK => {
                    println!("OK");
                    break;
                },
                In3Ret::WAITING => {
                    println!("WAITING");
                    //res = in3.send(&mut ctx);
                },
                _ => {
                    println!("detault");
                }
            }
        }
        
        println!("end");
}
fn rpc_call(){

}
fn main(){
   let future = sendt(); 
   block_on(future);
    //send_request();

}



