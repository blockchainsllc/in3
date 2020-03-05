/// getting a Transaction with or without API

import in3.*;
import in3.eth1.*;
import java.math.BigInteger;
import java.util.*;

public class GetTransaction {

  static String TXN_HASH = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e";

  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

    Transaction txn = getTransactionAPI(in3);
    System.out.println("Transaction API #blockNumber: " + txn.getBlockNumber());

    System.out.println("Transaction RPC :" + getTransactionRPC(in3));
  }

  static Transaction getTransactionAPI(IN3 in3) {
    return in3.getEth1API().getTransactionByHash(TXN_HASH);
  }

  static String getTransactionRPC(IN3 in3) {
    return in3.sendRPC("eth_getTransactionByHash", new Object[] {TXN_HASH});
  }
}