## Overview

The C implementation of the Incubed client is prepared and optimized to run on small embedded devices. Because each device is different, we prepare different modules that should be combined. This allows us to only generate the code needed and reduce requirements for flash and memory.

This is why Incubed consists of different modules. While the core module is always required, additional functions will be prepared by different modules:

### Verifier

Incubed is a minimal verification client, which means that each response needs to be verifiable. Depending on the expected requests and responses, you need to carefully choose which verifier you may need to register. For Ethereum, we have developed three modules:

1. [nano](#module-eth-nano): a minimal module only able to verify transaction receipts (`eth_getTransactionReceipt`).
2. [basic](#module-eth-basic): module able to verify almost all other standard RPC functions (except `eth_call`).
3. [full](#module-eth-full): module able to verify standard RPC functions. It also implements a full EVM to handle `eth_call`.

Depending on the module, you need to register the verifier before using it. This is done by calling the `in3_register...` function like [in3_register_eth_full()](#in3-register-eth-full).

### Transport

To verify responses, you need to be able to send requests. The way to handle them depends heavily on your hardware capabilities. For example, if your device only supports Bluetooth, you may use this connection to deliver the request to a device with an existing internet connection and get the response in the same way, but if your device is able to use a direct internet connection, you may use a curl-library to execute them. This is why the core client only defines function pointer [in3_transport_send](#in3-transport-send), which must handle the requests.

At the moment we offer these modules; other implementations are supported by different hardware modules.

1. [curl](#module-transport-curl): module with a dependency on curl, which executes these requests and supports HTTPS. This module runs a standard OS with curl installed.

### API

While Incubed operates on JSON-RPC level, as a developer, you might want to use a better-structured API to prepare these requests for you. These APIs are optional but make life easier:

1. [**eth**](#module-eth-api): This module offers all standard RPC functions as described in the [Ethereum JSON-RPC Specification](https://github.com/ethereum/wiki/wiki/JSON-RPC). In addition, it allows you to sign and encode/decode calls and transactions.
2. [**usn**](#module-usn-api): This module offers basic USN functions like renting, event handling, and message verification.
