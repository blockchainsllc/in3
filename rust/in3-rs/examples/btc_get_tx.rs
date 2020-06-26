extern crate in3;

use async_std::task;

use in3::btc::*;
use in3::eth1::Hash;
use in3::prelude::*;

fn main() {
    let mut btc_api = Api::new(Client::new(chain::BTC));

    //`get_transaction_bytes` is an asynchrous request (due to the internal C library). Therefore to block execution
    //we use async_std's block_on function
    let txid: Hash = serde_json::from_str::<Hash>(
        r#""0x83ce5041679c75721ec7135e0ebeeae52636cfcb4844dbdccf86644df88da8c1""#,
    )
    .unwrap();
    let tx_data = task::block_on(btc_api.get_transaction_bytes(txid)).unwrap();

    print_to_hex(tx_data.0)
}

fn print_to_hex(data: Vec<u8>) {
    print!("0x");
    for byte in data.iter() {
        print!("{:x}", byte);
    }
    print!("\n");
}
