# Examples


-  [get_block_api](./get_block_api.ts)
   Reads the latest block by calling IN3's web3.js-compatible eth API.
Read the eth api from web3.js docs: https://web3js.readthedocs.io/en/v1.3.0/web3-eth.html

-  [get_block_rpc](./get_block_rpc.ts)
   Reads the latest block by calling IN3's internal RPC to the WASM core.
Learn other exclusive IN3 RPC calls here: https://in3.readthedocs.io/en/develop/rpc.html

-  [register_pugin](./register_pugin.ts)
   
-  [use_web3](./use_web3.ts)
   use IN3 as Web3Provider in web3.js

-  [in3_in_browser](./in3_in_browser.html)
   use IN3 in html 

### Building

In order to run those examples, you need to install in3 and typescript first.
The build.sh will do this and the run the tsc-compiler

```sh
./build.sh
```

In order to run a example use

```
node build/get_block_api.ts
```
