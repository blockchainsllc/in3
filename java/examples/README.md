# Examples


-  [CallFunction](./CallFunction.java)
   Calling Functions of Contracts

-  [Configure](./Configure.java)
   Changing the default configuration

-  [GetBalance](./GetBalance.java)
   getting the Balance with or without API

-  [GetBlockAPI](./GetBlockAPI.java)
   getting a block with API

-  [GetBlockRPC](./GetBlockRPC.java)
   getting a block without API

-  [GetTransaction](./GetTransaction.java)
   getting a Transaction with or without API

-  [GetTransactionReceipt](./GetTransactionReceipt.java)
   getting a TransactionReceipt with or without API

-  [SendTransaction](./SendTransaction.java)
   Sending Transactions

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

