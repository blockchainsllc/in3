## Overview

The C implementation of the Incubed client is prepared and optimized to run on small embedded devices. Because each device is different, we prepare different modules that should be combined. This allows us to only generate the code needed and reduce requirements for flash and memory.

### Why C?

We have been asked a lot, why we implemented Incubed in C and not in Rust. 
When we started Incubed we began with a feasibility test and wrote the client in TypeScript. Once we confirmed it was working, we wanted to provide a minimal  verifaction client for embedded devices. And yes, we actually wanted to do it in Rust, since Rust offers a lot of safety-features (like the memory-management at compiletime, thread-safety, ...), but after considering a lot of different aspects we made a pragmatic desicion to use C.

These are the reasons why:

#### Support for embedded devices.

As of today almost all toolchain used in the embedded world are build for C. Even though Rust may be able to still use some, there are a lot of issues.
Quote from [rust-embedded.org](https://docs.rust-embedded.org/book/interoperability/#interoperability-with-rtoss):

*Integrating Rust with an RTOS such as FreeRTOS or ChibiOS is still a work in progress; especially calling RTOS functions from Rust can be tricky.*

This may change in the future, but C is so dominant, that chances of Rust taking over the embedded development completly is low.

#### Portability 

C is the most portable programming language. Rust actually has a pretty admirable selection of supported targets for a new language (thanks mostly to LLVM), but it pales in comparison to C, which runs on almost everything. A new CPU architecture or operating system can barely be considered to exist until it has a C compiler. And once it does, it unlocks access to a vast repository of software written in C. Many other programming languages, such as Ruby and Python, are implemented in C and you get those for free too.

Most programing language have very good support for calling c-function in a shared library (like ctypes in python or cgo in golang) or even support integration of C code directly like [android studio](https://developer.android.com/studio/projects/add-native-code) does.

#### Integration in existing projects

Since especially embedded systems are usually written in C/C++, offering a pure C-Implementation makes it easy for these projects to use Incubed, since they do not have to change their toolchain.


Even though we may not be able to use a lot of great features Rust offers by going with C, it allows to reach the goal to easily integrate with a lot of projects. For the future we might port the incubed to Rust if we see a demand or chance for the same support as C has today.


### Modules

Incubed consists of different modules. While the core module is always required, additional functions will be prepared by different modules.

```

digraph "GG" {
    graph [ rankdir = "RL" ]
    node [
      fontsize = "12"
      fontname="Helvetica"
      shape="ellipse"
    ];

    subgraph cluster_transport {
        label="Transports"  color=lightblue  style=filled
        transport_http;
        transport_curl;
        
    }
    
    
    evm;
    tommath;
    
    subgraph cluster_verifier {
        label="Verifiers"  color=lightblue  style=filled
        eth_basic;
        eth_full;
        eth_nano;
        btc;
    }
    subgraph cluster_bindings {
        label="Bindings"  color=lightblue  style=filled
        wasm;
        java;
        python;
        
    }
    subgraph cluster_api {
        label="APIs"  color=lightblue  style=filled
        eth_api;
        usn_api;
        
    }
        
    core;
    segger_rtt;
    crypto;
    core -> segger_rtt;
    core -> crypto // core -> crypto
    eth_api -> eth_nano // eth_api -> eth_nano
    eth_nano -> core // eth_nano -> core
    btc -> core // eth_nano -> core
    eth_basic -> eth_nano // eth_basic -> eth_nano
    eth_full -> evm // eth_full -> evm
    evm -> eth_basic // evm -> eth_basic
    evm -> tommath // evm -> tommath
    transport_http -> core // transport_http -> core
    transport_curl -> core // transport_http -> core
    usn_api -> core // usn_api -> core
    
    java -> core // usn_api -> core
    python -> core // usn_api -> core
    wasm -> core // usn_api -> core
}

```

#### Verifier

Incubed is a minimal verification client, which means that each response needs to be verifiable. Depending on the expected requests and responses, you need to carefully choose which verifier you may need to register. For Ethereum, we have developed three modules:

1. [eth_nano](#module-eth-nano): a minimal module only able to verify transaction receipts (`eth_getTransactionReceipt`).
2. [eth_basic](#module-eth-basic): module able to verify almost all other standard RPC functions (except `eth_call`).
3. [eth_full](#module-eth-full): module able to verify standard RPC functions. It also implements a full EVM to handle `eth_call`.
3. [btc](#module-btc): module able to verify bitcoin or bitcoin based chains.

Depending on the module, you need to register the verifier before using it. This is done by calling the `in3_register...` function like [in3_register_eth_full()](#in3-register-eth-full).

#### Transport

To verify responses, you need to be able to send requests. The way to handle them depends heavily on your hardware capabilities. For example, if your device only supports Bluetooth, you may use this connection to deliver the request to a device with an existing internet connection and get the response in the same way, but if your device is able to use a direct internet connection, you may use a curl-library to execute them. This is why the core client only defines function pointer [in3_transport_send](#in3-transport-send), which must handle the requests.

At the moment we offer these modules; other implementations are supported by different hardware modules.

1. [transport_curl](#module-transport-curl): module with a dependency on curl, which executes these requests and supports HTTPS. This module runs a standard OS with curl installed.
2. [transport_http](#module-transport-http): module with no dependency, but a very basic http-implementation (no https-support)

#### API

While Incubed operates on JSON-RPC level, as a developer, you might want to use a better-structured API to prepare these requests for you. These APIs are optional but make life easier:

1. [**eth**](#module-eth-api): This module offers all standard RPC functions as described in the [Ethereum JSON-RPC Specification](https://github.com/ethereum/wiki/wiki/JSON-RPC). In addition, it allows you to sign and encode/decode calls and transactions.
2. [**usn**](#module-usn-api): This module offers basic USN functions like renting, event handling, and message verification.
