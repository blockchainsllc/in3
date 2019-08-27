import java.util.*;
import in3.*;
import in3.eth1.*;
import java.math.BigInteger;

public class GetBlockRPC {
  //
  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = new IN3();

    // configure
    in3.setChainId(0x1); // set it to mainnet (which is also the default)

    // read the latest Block without the Transactions.
    String result = in3.sendRPC("eth_getBlockByNumber", new Object[] { "latest", false });

    // print the json-data
    System.out.println("current Block : " + result);
  }

}