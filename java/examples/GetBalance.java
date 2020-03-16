/// getting the Balance with or without API

import in3.*;
import in3.eth1.*;
import java.math.BigInteger;
import java.util.*;

public class GetBalance {

  static String AC_ADDR = "0xc94770007dda54cF92009BFF0dE90c06F603a09f";

  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

    System.out.println("Balance API" + getBalanceAPI(in3).longValue());

    System.out.println("Balance RPC " + getBalanceRPC(in3));
  }

  static BigInteger getBalanceAPI(IN3 in3) {
    return in3.getEth1API().getBalance(AC_ADDR, Block.LATEST);
  }

  static String getBalanceRPC(IN3 in3) {
    return in3.sendRPC("eth_getBalance", new Object[] {AC_ADDR, "latest"});
  }
}
