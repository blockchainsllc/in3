## Incubed - from Typescript to WASM

When we started to implement Incubed, it all began with a Proof of Concept. This was implemented in Typescript and showed that it was possible to verify all important ethereum rpc-requests. Out of thie PoC we continued to develop the first release of the Typescript Client at Devcon 2017. But the goal of incubed was always to be able to run in embedded devices with minimal specs (256kB RAM), so we started to implement the incubed client in C from scratch.

### Why C? (not Rust?)

Don't get me wrong, I love Rust and in the beginning this seemed the best way to implement it, but when looking at our target platforms (embeded devices) you soon find out that almost all of them use C or C++. It would be way easier to get a C-developer to inculde a C-Library than to change its toolchain to include Rust. Also at that time Rust was not ready for embedded yet. 
„Integrating Rust with an RTOS such as FreeRTOS or ChibiOS is still a work in progress; especially calling RTOS functions from Rust can be tricky.“ (https://rust-embedded.org)
When we looked at Stackoverflow Developer Survey 2019 for the most loved Language, 83% of developers made Rust the #1, but looking at the results for actually usage, only 3% used Rust in their products.
Another reason was the fact, that if you write code in C it will run everywhere. Especially embedded devices like to come with their own toolchain only supporting C/C++.

### Size matters

While most Dektop-PCs are powerful enough so you wouldn't care how big a library is, there are still 2 platforms, where size matters a lot:
1. Browsers - when including it in a webapp each user needs to download the code. So having a small library means a faster and better user experience.
2. Embedded Devices - if you only have 1MB flash memory you simply can't use huge libraries.

The smallest possible incubed client is around 300kb including a full evm and running on devices with about 100kB RAM. 

### Emscripten

With Incubed implemented in C we used emscripten to compile it to WASM. This allows us to run almost at native speed in the browser. While almost all browsers support WASM, yet there are still JS-Engines without WebAssembly-Support. Luckely emscripten is able to compile it to asmjs as well, which allows us to also run ( a bit slower and bigger ) even there (which by the way includes react native !).

### Security - no dependencies

Packing all in one wasm-file is not only creating a very compact size (The minimal wasm-size is 162kB), but also is a lot safer. 
When you look at your node_modules-folder after installing the old typescript-client, you would find 267 packages from various authors. 
if you do the same with the wasm-client version 3.2 now, you will find only one folder : `in3`. Yes, We are very proud of our library with ZERO dependencies!
Why are dependencies bad? According to a security survey by npm, 77% of respondents were concerned with the security of OSS/third-party code. Especially when writing security critical applications, auditing all even nested dependencies is very hard. (Remember when an innocent package like [event-stream](https://snyk.io/blog/malicious-code-found-in-npm-package-event-stream/) became malicious and got downloaded 8 Million times? )

## How to migrate?

For the WASM-Client we tried to keep the API as close as possible to the old TypeScript-Version, but due to new features and some WASM-specific behaviors there are some changes you need to be aware of:

1. WASM loads async
    Since WebAssembly is always loaded async, some function (like special util-functions) may not be available if you execute code directly after requiring the client. For all those cases the Incubed-Client now offers a onInit-function, which also returns a promis with the result. ( his is not be needed for normale rpc-request, since they are already async)

    ```js
    import IN3 from 'in3'

    // the code will be executed as soon as the client is ready (wasm loaded)
    IN3.onInit(()=>{
        console.log(' Address :  ',  IN3.util.toChecksumAddress('0xb3b7874f13387d44a3398d298B075b7a3505d8d4'))
    })
    ```

2. Freeing Memory
    As every C/C++-Developer knows, in order to avoid memory leaks, you need to free memory after usage. While it may not make a big difference if you only use one instance, if you create a temp instance, make sure to call `.free()` afterwards. 

3. We removed a lot of internal functions from the Client, like
    - `getChainContext()`
    - `updateWhiteListNodes()`
    - `updateNodeList()`
    - `call()`
    - `verifyResponse()`
    - `clearStats()`
    - `addListener()`

    On the other hand we also added some useful functions, like
    - `setConfig(config: Partial<IN3Config>)` - changes (partial) configurations
    - `execLocal(method: string, params?: any[])` - to execute a rpc request synchronously directly in the client, like ( `web3_version` or )

4. Changes in the Eth-API
    We extended the Eth-API (`in3.eth`) and added the following functions.

    - `accounts` - the Account API for adding signer keys
    - `toWei()` - converts any value and unit to wei as hex.
    - `resolveENS()` - ENS-Resolver
    - `web3ContractAt()` - creates a instance of a Contract with the same methods as web3 would do
    - `contractAt()`- creates a instance of an contract with the same methods as ethers would do

5. New APIs
    - `btc`-  this API support verified Bitcoin-Responses ( we will have a seperate blogpost about it )
    - `zksync` - API for using a zksync-service



