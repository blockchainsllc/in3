## Installing



The Incubed Java client uses JNI in order to call native functions. That's why to use it you need to put the shared library in the path where java will be able to find it. 

The shared library (`in3.dll` (windows), `libin3.so` (linux) or `libin3.dylib` (osx) ), can either be downloaded (make sure you know your targetsystem) or build from sources.

like

```sh
java -Djava.library.path="path_to_in3;${env_var:PATH}" HelloIN3.class
```


###  Building

For building the shared library you need to enable java by using the `-DJAVA=true` flag:

```sh
git clone git@github.com:slockit/in3-core.git
mkdir -p in3-core/build
cd in3-core/build
cmake -DJAVA=true .. && make
```

You will find the `in3.jar` and the `libin3.so` in the build/lib - folder.

### Android

In order to use incubed in android simply follow this example:

https://github.com/SlockItEarlyAccess/in3-android-example


## Example

### Using Incubed directly

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

Incubed also offers a API for getting Information directly in a structured way.

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

        // create a API instance which uses our incubed.
        API api = new API(in3);

        // read the latest Block including all Transactions.
        Block latestBlock = api.getBlockByNumber(Block.LATEST, true);

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

       // create a API instance which uses our incubed.
       API api = new API(in3);

       // call a contract, which uses eth_call to get the result. 
       Object[] result = (Object[]) api.call(                                   // call a function of a contract
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

