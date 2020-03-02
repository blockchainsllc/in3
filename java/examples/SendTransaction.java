
/// Sending Transactions

// In order to send, you need a Signer. The SimpleWallet class is a basic implementation which can be used.

package in3;

import in3.*;
import in3.eth1.*;
import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

public class SendTransaction {
  //
  public static void main(String[] args) throws IOException {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

    // create a wallet managing the private keys
    SimpleWallet wallet = new SimpleWallet();

    // add accounts by adding the private keys
    String keyFile      = "myKey.json";
    String myPassphrase = "<secrect>";

    // read the keyfile and decoded the private key
    String account = wallet.addKeyStore(
        Files.readString(Paths.get(keyFile)),
        myPassphrase);

    // use the wallet as signer
    in3.setSigner(wallet);

    String     receipient = "0x1234567890123456789012345678901234567890";
    BigInteger value      = BigInteger.valueOf(100000);

    // create a Transaction
    TransactionRequest tx = new TransactionRequest();
    tx.setFrom(account);
    tx.setTo("0x1234567890123456789012345678901234567890");
    tx.setFunction("transfer(address,uint256)");
    tx.setParams(new Object[] {receipient, value});

    String txHash = in3.getEth1API().sendTransaction(tx);

    System.out.println("Transaction sent with hash = " + txHash);
  }
}
