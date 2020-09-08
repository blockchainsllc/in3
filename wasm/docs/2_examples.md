## Examples

### get_block_rpc

source : [in3-c/wasm/examples/get_block_rpc.js](https://github.com/slockit/in3-c/blob/master/wasm/examples/get_block_rpc.js)

read block as rpc


```js
/// read block as rpc

const IN3 = require('in3-wasm')

async function showLatestBlock() {
    // create new incubed instance
    var c = new IN3()

    await c.setConfig({
        chainId: 0x5 // use goerli
    })

    // send raw RPC-Request (this would throw if the response contains an error)
    const lastBlockResponse = await c.sendRPC('eth_getBlockByNumber', ['latest', false])

    console.log("latest Block: ", JSON.stringify(lastBlockResponse, null, 2))

    // clean up
    c.free()

}

showLatestBlock().catch(console.error)
```

### get_block_api

source : [in3-c/wasm/examples/get_block_api.ts](https://github.com/slockit/in3-c/blob/master/wasm/examples/get_block_api.ts)

read block with API


```js
/// read block with API

import { IN3 } from 'in3-wasm'

async function showLatestBlock() {
  // create new incubed instance
  const client = new IN3({
    chainId: 'goerli'
  })

  // send raw RPC-Request
  const lastBlock = await client.eth.getBlockByNumber()

  console.log("latest Block: ", JSON.stringify(lastBlock, null, 2))

  // clean up
  client.free()

}

showLatestBlock().catch(console.error)
```

### register_pugin

source : [in3-c/wasm/examples/register_pugin.ts](https://github.com/slockit/in3-c/blob/master/wasm/examples/register_pugin.ts)

register a custom plugin


```js
/// register a custom plugin

import { IN3, RPCRequest } from 'in3-wasm'
import * as crypto from 'crypto'

class Sha256Plugin {

  // this function will register for handling rpc-methods
  // only if we return something other then `undefined`, it will be taken as the result of the rpc.
  // if we don't return, the request will be forwarded to the incubed nodes
  handleRPC(c: IN3, request: RPCRequest): any {
    if (request.method === 'sha256') {
      // assert params
      if (request.params.length != 1 || typeof (request.params[0]) != 'string')
        throw new Error('Only one parameter with as string is expected!')

      // create hash
      const hash = crypto.createHash('sha256').update(Buffer.from(request.params[0], 'utf8')).digest()

      // return the result
      return '0x' + hash.toString('hex')
    }
  }

}



async function registerPlugin() {
  // create new incubed instance
  const client = new IN3()

  // register the plugin
  client.registerPlugin(new Sha256Plugin())

  // exeucte a rpc-call
  const result = await client.sendRPC("sha256", ["testdata"])

  console.log(" sha256: ", result)

  // clean up
  client.free()

}

registerPlugin().catch(console.error)
```

### use_web3

source : [in3-c/wasm/examples/use_web3.ts](https://github.com/slockit/in3-c/blob/master/wasm/examples/use_web3.ts)

use incubed as Web3Provider in web3js 


```js
/// use incubed as Web3Provider in web3js 

// import in3-Module
import { IN3 } from 'in3-wasm'
const Web3 = require('web3')

const in3 = new IN3({
    proof: 'standard',
    signatureCount: 1,
    requestCount: 1,
    chainId: 'mainnet',
    replaceLatestBlock: 10
})

// use the In3Client as Http-Provider
const web3 = new Web3(in3.createWeb3Provider());

(async () => {

    // use the web3
    const block = await web3.eth.getBlock('latest')
    console.log("Block : ", block)

})().catch(console.error);
```

### in3_in_browser

source : [in3-c/wasm/examples/in3_in_browser.html](https://github.com/slockit/in3-c/blob/master/wasm/examples/in3_in_browser.html)

use incubed directly in html 


```html
<!-- use incubed directly in html -->
<html>

<head>
    <script src="node_modules/in3-wasm/index.js"></script>
</head>

<body>
    IN3-Demo
    <div>
        result:
        <pre id="result"> ...waiting... </pre>
    </div>
    <script>
        var in3 = new IN3({ chainId: 0x1, replaceLatestBlock: 6, requestCount: 3 });
        in3.eth.getBlockByNumber('latest', false)
            .then(block => document.getElementById('result').innerHTML = JSON.stringify(block, null, 2))
            .catch(alert)
            .finally(() => in3.free())
    </script>
</body>

</html>
```


### Building 

In order to run those examples, you need to install in3-wasm and typescript first.
The build.sh will do this and the run the tsc-compiler

```sh
./build.sh
```

In order to run a example use

```
node build/get_block_api.ts
```

