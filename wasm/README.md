# Incubed client as WASM

## Installing

This client uses the in3-core sources compiled to wasm. The wasm is included into the js-file wich makes it easier to include the data.
This module has **no** dependencies! All it needs is included inta a wasm of about 300kB.

Installing incubed is as easy as installing any other module:

```
npm install --save in3-wasm
```

### Documentation

The complete documentation can be found https://in3.readthedocs.io/en/develop/ .

In case you want to run incubed within a react native app, you might face issues because wasm is not supported there yet. In this case you can use [in3-asmjs](https://www.npmjs.com/package/in3-asmjs), which has the same API, but runs on pure javascript (a bit slower and bigger, but full support everywhere).


### Using web3

```js

// import in3-Module
import In3Client from 'in3-wasm'
import * as web3 from 'web3'

const IN3Client c = new IN3Client({
    proof              : 'standard',
    signatureCount     : 1,
    chainId            : 'mainnet',
    replaceLatestBlock : 10
 })

// use the In3Client as Http-Provider
const web3 = new Web3(c)

(async () => {

    // use the web3
    const block = await web.eth.getBlockByNumber('latest')

})().catch(console.error);

```

### Without web3 (Direct API)


Incubed includes a light API, allowinng not only to use all RPC-Methods in a typesafe way, but also to sign transactions and call funnctions of a contract without the web3-library.

For more details see the [API-Doc](https://github.com/slockit/in3/blob/master/docs/api.md#type-api)

```js

// import in3-Module
import In3Client from 'in3-wasm'

// use the In3Client
const in3 = new In3Client({
    proof              : 'standard',
    signatureCount     : 1,
    chainId            : 'mainnet',
    replaceLatestBlock : 10
})

(async () => {

    // use the incubed directly
    const block = await in3.eth.getBlockByNumber('latest')

})().catch(console.error);

```

### Direct include in a website

```html
<html>

    <head>
        <script src="in3.js"></script>
    </head>

    <body>
        IN3-Demo
        <div>
            result:
            <pre id="result">
                ...waiting...
            </pre>
        </div>
        <script>
            var c = new IN3({ 
                   chainId: 0x5 
            })
            c.send({ method: 'eth_getBlockByNumber', params: ['latest', false] })
                .then(r => document.getElementById('result').innerHTML = JSON.stringify(r, null, 2))
                .catch(alert)
        </script>
    </body>

</html>
```


### LICENSE

#### COMMERCIAL LICENSE 

Licensees holding a valid commercial license may use this software in accordance 
with the commercial license agreement provided with the Software or, alternatively, 
in accordance with the terms contained in a written agreement between you and 
slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
information please contact slock.it at in3@slock.it.
	
Alternatively, this software may be used under the AGPL license as follows:
   
#### AGPL LICENSE 

This program is free software: you can redistribute it and/or modify it under the 
terms of the GNU Affero General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
[Permissions of this strong copyleft license are conditioned on making available 
complete source code of licensed works and modifications, which include larger 
works using a licensed work, under the same license. Copyright and license notices 
must be preserved. Contributors provide an express grant of patent rights.]
You should have received a copy of the GNU Affero General Public License along 
with this program. If not, see <https://www.gnu.org/licenses/>.