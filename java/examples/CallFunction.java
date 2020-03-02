/// Calling Functions of Contracts

// This Example shows how to call functions and use the decoded results. Here we get the struct from the registry.

import in3.*;
import in3.eth1.*;

public class CallFunction {
  //
  public static void main(String[] args) {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

    // call a contract, which uses eth_call to get the result.
    Object[] result = (Object[]) in3.getEth1API().call(                      // call a function of a contract
        "0x2736D225f85740f42D17987100dc8d58e9e16252",                        // address of the contract
        "servers(uint256):(string,address,uint256,uint256,uint256,address)", // function signature
        1);                                                                  // first argument, which is the index of the node we are looking for.

    System.out.println("url     : " + result[0]);
    System.out.println("owner   : " + result[1]);
    System.out.println("deposit : " + result[2]);
    System.out.println("props   : " + result[3]);
  }
}
