USN-Application with in3-core

```mermaid
sequenceDiagram

    participant Phone
    participant App
    participant in3

    App--xApp: wait for requests
    Phone->>App: ActionMessage
    App-->>+in3: Request the Receipt
    in3->>-Phone: prepared IN3-Request to fetch
    Phone->>+in3: IN3-Response with Receipt & Proof
    in3-->>-App: Verified Receipt
    App--xApp: verify permission 
    App--xApp: trigger gpio 
    App->>Phone: Response for Action

```

# Verification-Flow


```mermaid
graph TD
    
    req[Request] --> hasSignature{ has valid Signature ? }
    hasSignature -- yes --> isAction{ msgType == 'action' ? }
    hasSignature -- no --> reject[ send Error-Response ]
    
    isAction -- yes --> hasTransaction{ has TransactionHash ? }
    isAction -- no --> isInit{ msgType == 'init' ? }
    
    isInit -- yes --> isOwner{ no owner or signer=owner ? }
    isInit -- no --> reject

    isOwner -- yes --> setDeviceParams[ set owner, deviceId, contract, chainId ]
    isOwner -- no --> reject
    
    hasTransaction -- yes --> isCached{ Do we have a cached Receipt ?}
    hasTransaction -- no --> reject
    
    isCached -- yes --> hasEvent{Receipt has LogRented-Event?}
    isCached -- no --> in3_tx[ get a verified TransactionReceipt ] 
    
    in3_tx -. in3-request over ble .-> in3_tx

    in3_tx -- valid --> cache[Store Receipt in RAM]
    in3_tx -- error --> reject
    
    cache --> hasEvent

    hasEvent -- yes --> checkSignature{ signer == controller ? }
    hasEvent -- no --> reject
    
    checkSignature -- yes --> checkTime{ is timestamp between rentedFrom and rentedUntil? }
    checkSignature -- no --> reject
    
    checkTime -- yes --> checkDeviceId{ does the contract and deviceId match the stored? }
    checkTime -- no --> reject
    
    checkDeviceId -- yes --> triggerAction[Turn on GPIO]
    checkDeviceId -- no --> reject
    
    triggerAction --> sendSuccess[Send Success Response]
    triggerAction --> wait[wait 10s]
    wait --> turnOff[Turn off GPIO]
    
    
    
```