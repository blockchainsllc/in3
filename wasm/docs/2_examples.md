## Examples

### get_block_api

source : [in3-c/wasm/examples/get_block_api.ts](https://github.com/blockchainsllc/in3/blob/master/wasm/examples/get_block_api.ts)

Reads the latest block by calling IN3's web3.js-compatible eth API.
Read the eth api from web3.js docs: https://web3js.readthedocs.io/en/v1.3.0/web3-eth.html


```js
/// Reads the latest block by calling IN3's web3.js-compatible eth API.
/// Read the eth api from web3.js docs: https://web3js.readthedocs.io/en/v1.3.0/web3-eth.html
import { IN3 } from 'in3'

async function showLatestBlock() {

  // create new IN3 instance
  const client = new IN3({
    proof              : 'standard',
    signatureCount     : 1,
    chainId            : 'goerli'
  })

  const lastBlock = await client.eth.getBlockByNumber()

  console.log("latest Block: ", JSON.stringify(lastBlock, null, 2))

}

showLatestBlock().catch(console.error)

```

### get_block_rpc

source : [in3-c/wasm/examples/get_block_rpc.ts](https://github.com/blockchainsllc/in3/blob/master/wasm/examples/get_block_rpc.ts)

Reads the latest block by calling IN3's internal RPC to the WASM core.
Learn other exclusive IN3 RPC calls here: https://in3.readthedocs.io/en/develop/rpc.html


```js
/// Reads the latest block by calling IN3's internal RPC to the WASM core.
/// Learn other exclusive IN3 RPC calls here: https://in3.readthedocs.io/en/develop/rpc.html
import { IN3 } from 'in3'

async function showLatestBlock() {

    // create new IN3 instance
    var c = new IN3({
        chainId: '0x5' // use goerli
    })

    // make a RPC (this would throw if the response contains an error)
    const lastBlockResponse = await c.sendRPC('eth_getBlockByNumber', ['latest', false])

    console.log("latest Block: ", JSON.stringify(lastBlockResponse, null, 2))

}

showLatestBlock().catch(console.error)

```

### register_pugin

source : [in3-c/wasm/examples/register_pugin.ts](https://github.com/blockchainsllc/in3/blob/master/wasm/examples/register_pugin.ts)



```js
// Register a custom RPC for sha256 hashing using a plugin
// Read about IN3 Plugins in the docs: https://in3.readthedocs.io/en/develop/api-c.html#plugins
import { IN3, RPCRequest, IN3Plugin } from 'in3'
import * as crypto from 'crypto'

class Sha256Plugin<BigIntType, BufferType> implements IN3Plugin<BigIntType, BufferType> {

  // this function will register for handling rpc-methods
  // only if we return something other then `undefined`, it will be taken as the result of the rpc.
  // if we don't return, the request will be forwarded to the IN3 nodes
  handleRPC(client, request: RPCRequest): any {

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

  // create new IN3 instance
  const client = new IN3()

  // register the plugin
  client.registerPlugin(new Sha256Plugin())

  // exeucte a rpc-call
  const result = await client.sendRPC("sha256", ["testdata"])

  console.log(" sha256: ", result)

}

registerPlugin().catch(console.error)

```

### use_web3

source : [in3-c/wasm/examples/use_web3.ts](https://github.com/blockchainsllc/in3/blob/master/wasm/examples/use_web3.ts)

use IN3 as Web3Provider in web3.js


```js
/// use IN3 as Web3Provider in web3.js
import {IN3} from 'in3'

const Web3 = require('web3')

const in3 = new IN3({
    proof: 'standard',
    signatureCount: 1,
    requestCount: 1,
    chainId: 'goerli',
    replaceLatestBlock: 10
})

// Use IN3 network client as a Http-Provider
const web3 = new Web3(in3.createWeb3Provider());

(async () => {
    const block = await web3.eth.getBlock('latest')
    console.log("Block : ", block)
})().catch(console.error);

```

### in3_in_browser

source : [in3-c/wasm/examples/in3_in_browser.html](https://github.com/blockchainsllc/in3/blob/master/wasm/examples/in3_in_browser.html)

use IN3 in html 


```html
<!-- use IN3 in html -->
<html>

<head>
    <script src="node_modules/in3/index.js"></script>
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
    </script>
</body>

</html>

```


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
