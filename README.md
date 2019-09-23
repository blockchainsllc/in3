# INCUBED Client in C
 [![Forks](https://img.shields.io/github/forks/slockit/in3)](https://github.com/slockit/in3-c/network/members)
  [![Stars](https://img.shields.io/github/stars/slockit/in3)](https://github.com/slockit/in3-c/watchers)
  [![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://github.com/slockit/in3-c/blob/master/LICENSE.AGPL)
 
 INCUBED (in3) is a minimal verification client for blockchain networks, this version of in3 is written in C. 
 
 Most blockchains, such as Ethereum, require a client to connect to their blockchain network. Often, these clients 
 require a very high bandwidth or constant computation. While this is possible to perform on laptops or desktop systems, 
 mobile devices struggle to meet these requirements. Currently the solution of choice is to use a light client on mobile 
 devices. While this works for mobile phones, most IoT devices are unable to run light clients. Connecting an IoT device 
 to a remote node enables even low-performance IoT devices to be connected to blockchain. However, by using distinct 
 remote nodes the advantages of a decentralized network are undermined introducing a single point of failure.
 The Trustless Incentivized Remote Node Network, in short INCUBED, would make it possible to establish a 
 decentralized and secure network of remote nodes, enabling trustworthy and fast access to blockchain for a large number 
 of low-performance IoT and mobile devices.
 
 A more detailed explanation of in3 can be found [here](https://in3.readthedocs.io/en/develop/intro.html).
 
 Topics to include:
 1. image of how in3 works
 
 
 ## Installation and Usage
 |         | package manager           | Link  | Use case |
 | ------------- |:-------------:| -----:| ----:|
 | in3-client(C)      |  Ubuntu Launchpad     |  https://bit.ly/2m4y17b | It can be nicely integrated on IoT devices or any micro controllers |
 | | Docker Hub | https://hub.docker.com/r/slockit/in3  | Quick and easy way to get in3 client running
 | | Brew      |    TO ADD LATER | Easy to install on MacOS or linux/windows subsystems
 | | Release page | https://github.com/slockit/in3-c/releases | For directly playing with the binaries/deb/images
 
 ### Ubuntu Launchpad 
 There are 2 packages published to Ubuntu Launchpad: ```in3``` and ```in3-dev```. The package ```in3``` only installs the
 binary file and allows you to use in3 via command line. The package ```in3-dev``` would install the binary as well as 
 the library files, allowing you to use in3 not only via command line, but also inside your C programs by including the
 statically linked files. 
 
 #### Installation instructions for ```in3```:
 1. Add the slock.it ppa to your system with
 ```sudo add-apt-repository ppa:devops-slock-it/in3```
 2. Update the local sources ```sudo apt-get update```
 3. Install in3 with ```sudo apt-get install in3```

 #### Installation instructions for ```in3-dev```:
 1. Add the slock.it ppa to your system with
 ```sudo add-apt-repository ppa:devops-slock-it/in3```
 2. Update the local sources ```sudo apt-get update```
 3. Install in3 with ```sudo apt-get install in3-dev```
 
 ### Docker Hub
 #### Usage instructions:
 1. Pull the image from docker using ```docker pull slockit/in3```
 2. Run the client using: ```docker run -d -p 8545:8545  slockit/in3:latest --chainId=goerli```
 3. More parameters and their descriptions can be found [here](https://in3.readthedocs.io/en/develop/getting_started.html#as-docker-container). 
 
 ### Release page
 #### Usage instructions:
 1. Navigate to the in3-client [release page](https://github.com/slockit/in3-c/releases) on this github repo 
 2. Download the binary that matches your target system, read below for architecture specific information:
 
 ###### For WASM:
 ###### For C library:
 
 ### Brew
 #### Usage instructions:
 
 ## Example 
 ### CLI
  in3 can be used on the command line in this manner: ```in3 [options] method [arguments]```
  
  For example, to get block number, run: ```in3 eth_blockNumber``` in the console.
  
  A more detailed list with information on arguments can be found [here](https://in3.readthedocs.io/en/develop/api-cmd.html).
  
 ### C Code
 ```c
 #include <client/client.h> // the core client
 #include <eth_full.h>      // the full ethereum verifier containing the EVM
 #include <in3_curl.h>      // transport implementation
 
 // register verifiers, in this case a full verifier allowing eth_call
 in3_register_eth_full();
 
 // create new client
 in3_t* client = in3_new();
 
 // configure storage by using storage-functions from in3_curl, which store the cache in /home/<USER>/.in3
 in3_storage_handler_t storage_handler;
 storage_handler.get_item = storage_get_item;
 storage_handler.set_item = storage_set_item;
 
 client->cacheStorage = &storage_handler;
 
 // configure transport by using curl
 client->transport    = send_curl;
 
 // init cache by reading the nodelist from the cache >(if exists)
 in3_cache_init(client);
 
 // ready to use ...
 ```
 A more detailed example with information on how to call functions can be found [here](https://in3.readthedocs.io/en/develop/api-c.html#examples).


 ## Features
 
 |                            | in3  | Remote Client | Light Client | 
 | -------------------------- | :----------------: | :----------------: |  :----------------: |
 | Failsafe connection        |         ✔️         |     ❌     |  ✔️ |
 | Automatic Nodelist updates |         ✔️         |     ❌     |  ✔️ | 
 | Partial nodelist           |         ✔️         |     ❌     |  ✔️ |
 | Multi-chain support        |         ✔️         |      ✔️    |  ❌ |
 | Full verification of JSON-RPC methods   |         ✔️         |  ❌  | ❌  |
 | IPFS support               |         ✔️         |    ✔️    |  ❌ |
 | Caching support            |         ✔️         |    ❌      |  ❌ |
 | Proof-Levels               |         ✔️         |    ❌      |  ❌ |
 | POA Support                |         ✔️         |    ✔️    |  ✔️   |
 | Database setup size-minutes|        0-instant️   |    0-instant    |  ~50Mb-minutes️ |
 | Uses                       |         IoT devices,Mobile,browser️ |    Mobile,browser️️    |  PC,Laptop️   |
 
 ## Resources 
 
 * [C API reference](https://in3.readthedocs.io/en/develop/api-c.html)
 * [C examples](https://in3.readthedocs.io/en/develop/api-c.html#examples)
 * [Website](https://slock.it/incubed/) 
 * [ReadTheDocs](https://in3.readthedocs.io/en/develop/)
 * [Blog](https://blog.slock.it/)
 * [Incubed concept video by Christoph Jentzsch](https://www.youtube.com/watch?v=_vodQubed2A)
 * [Ethereum verification explained by Simon Jentzsch](https://www.youtube.com/watch?v=wlUlypmt6Oo)
 
 ## Contributors welcome!
 [![GitHub issues](https://img.shields.io/github/issues/slockit/in3-c)](https://github.com/slockit/in3/issues)
 [![GitHub contributors](https://img.shields.io/github/contributors/slockit/in3-c)](https://github.com/slockit/in3-c/graph/contributors)
 
 We at Slock.it believe in the power of the open source community. Feel free to open any issues you may come across, fork
  the repository and integrate in your own projects. You can reach us on various social media platforms for any questions
  and suggestions.  
 
 [![Twitter](https://img.shields.io/badge/Twitter-Page-blue)](https://twitter.com/slockitproject?s=17)
 [![Blog](https://img.shields.io/badge/Blog-Medium-blue)](https://blog.slock.it/)
 [![Youtube](https://img.shields.io/badge/Youtube-channel-blue)](https://www.youtube.com/channel/UCPOrzp3CZmdb5HJWxSjv4Ig)
 [![LinkedIn](https://img.shields.io/badge/Linkedin-page-blue)](https://www.linkedin.com/company/10327305
 )
 
 ## Got any questions?
 Send us an email at <a href="mailto:team-in3@slock.it">team-in3@slock.it</a>





                                                                                                                                                                                                                                                                                                                                                                                                                                                                 