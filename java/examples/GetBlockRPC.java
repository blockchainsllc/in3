/// getting a block without API

import in3.*;
import in3.eth1.*;
import java.math.BigInteger;
import java.util.*;

public class GetBlockRPC {
  //
  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also the default)

    // read the latest Block without the Transactions.
    String result = in3.sendRPC("eth_getBlockByNumber", new Object[] {"latest", false});

    // print the json-data
    System.out.println("current Block : " + result);
  }
}