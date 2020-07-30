
# rpc-api

## on boarding

### zksync_getAccountId

```js
req
{
   method: "zksync_getAccountId",
   params: [
       addresss //  sender address
   ]
}

res
{
    result: string // the account id
}
```


### zksync_depositToSyncFromEthereum

```js
req
{
   method: "zksync_depositToSyncFromEthereum",
   params: [
      depositTo: Address; //sender address 0xadf
      token: TokenLike; // token address 0x000
      amount: utils.BigNumberish; // 0x0 value
      ethTxOptions?: ethers.providers.TransactionRequest; // params
      approveDepositAmountForERC20?: boolean; // boolean
   ]
}

res
{
    result: {
        txReceipt
        priorityOpId
    } // the account id
}
```


### zksync_getPriorityOpStatus

```js
req
{
   method: "zksync_getPriorityOpStatus",
   params: [priorityOpId]
}

res
{
    result: {
          executed: boolean;
          block?: {
              blockNumber: number;
              committed: boolean;
              verified: boolean;
          };
    } // the account id
}
```

## Payment


### accountInfo



```js

// `account_info`
req
{
   method: "zksync_getState",
   params: [
         address // sender address
   ]
}

res
{
    result: {
            address: Address;
            id?: number;
            depositing: {
                balances: {
                    // Token are indexed by their symbol (e.g. "ETH")
                    [token: string]: {
                        // Sum of pending deposits for the token.
                        amount: utils.BigNumberish;
                        // Value denoting the block number when the funds are expected
                        // to be received by zkSync network.
                        expectedAcceptBlock: number;
                    };
                };
            };
            committed: {
                balances: {
                    // Token are indexed by their symbol (e.g. "ETH")
                    [token: string]: utils.BigNumberish;
                };
                nonce: number;
                pubKeyHash: PubKeyHash;
            };
            verified: {
                balances: {
                    // Token are indexed by their symbol (e.g. "ETH")
                    [token: string]: utils.BigNumberish;
                };
                nonce: number;
                pubKeyHash: PubKeyHash;
            };
    } // the account id
}
```


### zksync_xxSubmit

```js
req
{
   method: "zksync_txSubmit",
   params: [{
        to: Address;
        token: TokenLike;
        amount: utils.BigNumberish;
        fee?: utils.BigNumberish;
        nonce?: Nonce;
    }]
}

res
{
    result:       txHash // 
}
```

### zksync_txInfo

```js
req
{
   method: "zksync_txInfo",
   params: [txHash]
}

res
{
    result: {
        executed: boolean;
        success?: boolean;
        failReason?: string;
        block?: {
            blockNumber: number;
            committed: boolean;
            verified: boolean;
        };
    }
}
```


## withdraw

### zksync_withdrawToSyncFromEthereum


```js
req
{
   method: "zksync_withdrawToSyncFromEthereum",
   params: [{
        ethAddress: string; // sender address
        token: TokenLike; // addres
        amount: utils.BigNumberish; // amount
        fee?: utils.BigNumberish; //
        nonce?: Nonce;
    }]
}

res
{
    result: txHash
}
```



----------


# config

```js
{
  zksync: {
      provider: <url>

  }
} 

```c
typedef struct {
   char* url;
   addresst_t contract;
   
} zs_provider_t;
```