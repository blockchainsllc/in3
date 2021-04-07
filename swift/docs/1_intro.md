# API Reference Swift

The swift binding contains binaries are only available for macos and ios. ( for now)

## Install


TODO

## Usage

In order to use incubed, you need to add the In3-Package as dependency and import into your code:

```swift
import In3

// configure and create a client.
let in3 = try In3Config(chainId: "mainnet", requestCount:1).createClient()

// use the Eth-Api to execute requests
Eth(in3).getTransactionReceipt(hash: "0xe3f6f3a73bccd73b77a7b9e9096fe07b9341e7d1d8f1ad8b8e5207f2fe349fa0").observe(using: {
            switch $0 {
            case let .failure(err):
                print("Failed to get the tx : \(err)")
            case let .success( tx ):
                if let tx = tx {
                    print("Found tx with txhash \(tx.hash)")
                } else {
                    print("Tx does not exist")
                }
            }
        })
```
