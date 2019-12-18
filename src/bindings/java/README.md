## Installing



The Incubed Java client uses JNI in order to call native functions. But all the native-libraries are bundled inside the jar-file.
This jar file ha **no** dependencies and can even be used standalone: 

like

```sh
java -cp in3.jar in3.IN3 eth_getBlockByNumber latest false
```

### Downloading


The jar file can be downloaded from the latest release. [here](https://github.com/slockit/in3-c/releases).

Alternatively, If you wish to download Incubed using the maven package manager, add this to your pom.xml
```
<dependency>
  <groupId>it.slock</groupId>
  <artifactId>in3</artifactId>
  <version>2.21</version>
</dependency> 
```

After which, install in3 with ```mvn install ```.

###  Building

For building the shared library you need to enable java by using the `-DJAVA=true` flag:

```sh
git clone git@github.com:slockit/in3-core.git
mkdir -p in3-core/build
cd in3-core/build
cmake -DJAVA=true .. && make
```

You will find the `in3.jar` in the build/lib - folder.

### Android

In order to use Incubed in android simply follow these steps:

Step 1: Create a top-level CMakeLists.txt in android project inside app folder and link this to gradle. Follow the steps using this [guide](https://developer.android.com/studio/projects/gradle-external-native-builds) on howto link.

The Content of the `CMakeLists.txt` should look like this:

```sh

cmake_minimum_required(VERSION 3.4.1)

# turn off FAST_MATH in the evm.
ADD_DEFINITIONS(-DIN3_MATH_LITE)

# loop through the required module and cretae the build-folders
foreach(module 
  core 
  verifier/eth1/nano 
  verifier/eth1/evm 
  verifier/eth1/basic 
  verifier/eth1/full 
  bindings/java
  third-party/crypto 
  third-party/tommath 
  api/eth1)
        file(MAKE_DIRECTORY in3-core/src/${module}/outputs)
        add_subdirectory( in3-core/src/${module} in3-core/src/${module}/outputs )
endforeach()

```

Step 2: clone [in3-core](https://github.com/slockit/in3-c.git) into the `app`-folder or use this script to clone and update in3:

```sh
#!/usr/bin/env sh

#github-url for in3-core
IN3_SRC=https://github.com/slockit/in3-c.git

cd app

# if it exists we only call git pull
if [ -d in3-core ]; then
    cd in3-core
    git pull
    cd ..
else
# if not we clone it
    git clone $IN3_SRC
fi


# copy the java-sources to the main java path
cp -r in3-core/src/bindings/java/in3 src/main/java/
# but not the native libs, since these will be build
rm -rf src/main/java/in3/native
```

Step 3: Use methods available in app/src/main/java/in3/IN3.java from android activity to access IN3 functions.


Here is example how to use it:

https://github.com/slockit/in3-example-android


## Examples

### Using in3 directly

```java
import in3.IN3;

public class HelloIN3 {  
   // 
   public static void main(String[] args) {
       String blockNumber = args[0]; 

       // create incubed
       IN3 in3 = new IN3();

       // configure
       in3.setChainId(0x1);  // set it to mainnet (which is also dthe default)

       // execute the request
       String jsonResult = in3.sendRPC("eth_getBlockByNumber",new Object[]{ blockNumber ,true});

       ....
   }
}
```

### Using the API

in3 also offers a API for getting Information directly in a structured way.

#### Reading Blocks


```java
import java.util.*;
import in3.*;
import in3.eth1.*;

public class HelloIN3 {  
   // 
    public static void main(String[] args) throws Exception {
        // create incubed
        IN3 in3 = new IN3();

        // configure
        in3.setChainId(0x1); // set it to mainnet (which is also dthe default)

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


#### Calling Functions of Contracts

This Example shows how to call functions and use the decoded results. Here we get the struct from the registry.


```java
import in3.*;
import in3.eth1.*;

public class HelloIN3 {  
   // 
   public static void main(String[] args) {
       // create incubed
       IN3 in3 = new IN3();

       // configure
       in3.setChainId(0x1);  // set it to mainnet (which is also dthe default)

       // call a contract, which uses eth_call to get the result. 
       Object[] result = (Object[]) in3.getEth1API().call(                                   // call a function of a contract
            "0x2736D225f85740f42D17987100dc8d58e9e16252",                       // address of the contract
            "servers(uint256):(string,address,uint256,uint256,uint256,address)",// function signature
            1);                                                                 // first argument, which is the index of the node we are looking for.

        System.out.println("url     : " + result[0]);
        System.out.println("owner   : " + result[1]);
        System.out.println("deposit : " + result[2]);
        System.out.println("props   : " + result[3]);


       ....
   }
}
```

#### Sending Transactions

In order to send, you need a Signer. The SimpleWallet class is a basic implementation which can be used.

```java 
package in3;

import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import in3.*;
import in3.eth1.*;

public class Example {
    //
    public static void main(String[] args) throws IOException{
        // create incubed
        IN3 in3 = new IN3();

        // configure
        in3.setChainId(0x1); // set it to mainnet (which is also dthe default)

        // create a wallet managing the private keys
        SimpleWallet wallet = new SimpleWallet();

        // add accounts by adding the private keys
        String keyFile = "myKey.json";
        String myPassphrase = "<secrect>";

        // read the keyfile and decoded the private key
        String account = wallet.addKeyStore(
                Files.readString(Paths.get(keyFile)),
                myPassphrase);

        // use the wallet as signer
        in3.setSigner(wallet);

        String receipient = "0x1234567890123456789012345678901234567890";
        BigInteger value = BigInteger.valueOf(100000);

        // create a Transaction
        TransactionRequest tx = new TransactionRequest();
        tx.from = account;
        tx.to = "0x1234567890123456789012345678901234567890";
        tx.function = "transfer(address,uint256)";
        tx.params = new Object[] { receipient, value };

        String txHash = in3.getEth1API().sendTransaction(tx);

        System.out.println("Transaction sent with hash = " + txHash);

    }
}
```
