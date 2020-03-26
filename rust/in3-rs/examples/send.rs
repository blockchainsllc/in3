use in3_sys::*;
use std::ffi;
use in3_sys::in3_ret_t;
extern crate in3;
use in3::prelude::*;
fn send_request() {
        let mut in3 = Client::new();
        let mut req_s = String::from("{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
        //let mut req_s = String::from(r#"{"method":"eth_blockNumber","params":[]}"#);
        let mut ctx = Ctx::new(&mut in3, req_s);
        let _ = in3.execute(&mut ctx);
        let mut request = Request::new(&mut ctx);
        loop {
            let mut res = in3.execute(&mut ctx);
            match res {
                In3Ret::OK => {
                    println!("OK");
                    break;
                },
                In3Ret::WAITING => {
                    println!("WAITING");
                    res = in3.send(&mut ctx);
                },
                _ => {
                    println!("detault");
                }
            }
        }
        
        println!("end");
}
fn main(){
    send_request();
}



