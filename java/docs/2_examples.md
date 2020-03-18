# Examples

### CallFunction

source : [in3-c/java/examples/CallFunction.java](https://github.com/slockit/in3-c/blob/master/java/examples/CallFunction.java)

Calling Functions of Contracts


```java
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

```

### Configure

source : [in3-c/java/examples/Configure.java](https://github.com/slockit/in3-c/blob/master/java/examples/Configure.java)

Changing the default configuration


```java
/// Changing the default configuration

// In order to change the default configuration, just use the classes inside in3.config package.

package in3;

import in3.*;
import in3.config.*;
import in3.eth1.Block;

public class Configure {
  //
  public static void main(String[] args) {
    // create incubed client
    IN3 in3 = IN3.forChain(Chain.GOERLI); // set it to goerli

    // Setup a Configuration object for the client
    ClientConfiguration clientConfig = in3.getConfig();
    clientConfig.setReplaceLatestBlock(6); // define that latest will be -6
    clientConfig.setAutoUpdateList(false); // prevents node automatic update
    clientConfig.setMaxAttempts(1);        // sets max attempts to 1 before giving up
    clientConfig.setProof(Proof.none);     // does not require proof (not recommended)

    // Setup the ChainConfiguration object for the nodes on a certain chain
    ChainConfiguration chainConfiguration = new ChainConfiguration(Chain.GOERLI, clientConfig);
    chainConfiguration.setNeedsUpdate(false);
    chainConfiguration.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    chainConfiguration.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

    in3.setConfig(clientConfig);

    Block block = in3.getEth1API().getBlockByNumber(Block.LATEST, true);
    System.out.println(block.getHash());
  }
}
```

### GetBalance

source : [in3-c/java/examples/GetBalance.java](https://github.com/slockit/in3-c/blob/master/java/examples/GetBalance.java)

getting the Balance with or without API


```java
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

```

### GetBlockAPI

source : [in3-c/java/examples/GetBlockAPI.java](https://github.com/slockit/in3-c/blob/master/java/examples/GetBlockAPI.java)

getting a block with API


```java
/// getting a block with API

import in3.*;
import in3.eth1.*;
import java.math.BigInteger;
import java.util.*;

public class GetBlockAPI {
  //
  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

    // read the latest Block including all Transactions.
    Block latestBlock = in3.getEth1API().getBlockByNumber(Block.LATEST, true);

    // Use the getters to retrieve all containing data
    System.out.println("current BlockNumber : " + latestBlock.getNumber());
    System.out.println("minded at : " + new Date(latestBlock.getTimeStamp()) + " by " + latestBlock.getAuthor());

    // get all Transaction of the Block
    Transaction[] transactions = latestBlock.getTransactions();

    BigInteger sum = BigInteger.valueOf(0);
    for (int i = 0; i < transactions.length; i++)
      sum = sum.add(transactions[i].getValue());

    System.out.println("total Value transfered in all Transactions : " + sum + " wei");
  }
}
```

### GetBlockRPC

source : [in3-c/java/examples/GetBlockRPC.java](https://github.com/slockit/in3-c/blob/master/java/examples/GetBlockRPC.java)

getting a block without API


```java
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
```

### GetTransaction

source : [in3-c/java/examples/GetTransaction.java](https://github.com/slockit/in3-c/blob/master/java/examples/GetTransaction.java)

getting a Transaction with or without API


```java
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
```

### GetTransactionReceipt

source : [in3-c/java/examples/GetTransactionReceipt.java](https://github.com/slockit/in3-c/blob/master/java/examples/GetTransactionReceipt.java)

getting a TransactionReceipt with or without API


```java
/// getting a TransactionReceipt with or without API

import in3.*;
import in3.eth1.*;
import java.math.BigInteger;
import java.util.*;

public class GetTransactionReceipt {
  static String TRANSACTION_HASH = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e";

  //
  public static void main(String[] args) throws Exception {
    // create incubed
    IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also the default)

    TransactionReceipt txn = getTransactionReceiptAPI(in3);
    System.out.println("TransactionRerceipt API : for txIndex " + txn.getTransactionIndex() + " Block num " + txn.getBlockNumber() + " Gas used " + txn.getGasUsed() + " status " + txn.getStatus());

    System.out.println("TransactionReceipt RPC : " + getTransactionReceiptRPC(in3));
  }

  static TransactionReceipt getTransactionReceiptAPI(IN3 in3) {
    return in3.getEth1API().getTransactionReceipt(TRANSACTION_HASH);
  }

  static String getTransactionReceiptRPC(IN3 in3) {
    return in3.sendRPC("eth_getTransactionReceipt", new Object[] {TRANSACTION_HASH});
  }
}

```

### SendTransaction

source : [in3-c/java/examples/SendTransaction.java](https://github.com/slockit/in3-c/blob/master/java/examples/SendTransaction.java)

Sending Transactions


```java

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

```


### Building 

In order to run those examples, you only need a Java SDK installed.

```
./build.sh
```

will build all examples in this directory.

In order to run a example use

```
java -cp $IN3/build/lib/in3.jar:. GetBlockAPI
```

