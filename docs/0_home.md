## Overview

The C Implementation of the incubed client is prepared and optimized to run on small embedded devices. Because each device is different, we prepare different modules which should be combined. This allowes us to only generate the code needed and so reduce the requirements for flash and memory.

This is why the incubed is combind of different modules. While the core-module is always required additional functions will be prepared by differen modules:

### Verifier

Incubed is a minimal verifaction client, which means, each response needs to be verifable. Depending on the expected requests and responses you need to carefully choose which verifier you may need to register. For ethereum we have developed 3 Modules:

- [nano](#module-eth-nano) : a Minimal module only able to verify transaction receipts ( `eth_getTransactionReceipt`)
- [basic](#module-eth-basic) :  module able to  verify almost all other standard rpc-function. (except `eth_call`)
- [full](#module-eth-full) :  module able to verify standard rpc-function. It also implements a full EVM in order to handle `eth_call`

Depending on the module you need to register the verifier before using it. This is done by calling the `in3_register...` function like [in3_register_eth_full()](#in3-register-eth-full).


### Transport

In order to verify responses, you need to able to send requests. The way to handle them depend heavily on your hardware capabilities. For example, if your device only supports bluetooth, you may use this connection to deliver the request to a device with a existing internet connection and get the response in the same way, but if your device is able to use a direct internet connection, you may use a curl-library to execxute them. That's why the core client only defines a function pointer [in3_transport_send](#in3-transport-send) which must handle the requests.

At the moment we offer these modules, other implementation by supported inside different hardware-modules.

- [curl](#module-transport-curl) :  module with a dependency to curl which executes these requests with curl, also supporting  HTTPS. This modules is supposed to run an standard os with curl installed.

### API

While incubed operates on JSON-RPC-Level, as a developer you might want to use a better structed API preparing these requests for you. These APIs are optional but make life easier:

- [**eth**](#module-eth-api) : This module offers all standard RPC-Functions as descriped in the [Ethereum JSON-RPC Specification](https://github.com/ethereum/wiki/wiki/JSON-RPC). In addition it allows you to sign and encode/decode calls and transactions.
- [**usn**](#module-usn-api) : This module offers basic USN-function like renting or event-handling and message-verifaction.



