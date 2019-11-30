# Incubed client as WASM

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
    requestCount       : 2,
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
    requestCount       : 2,
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

All rights reserved to Slock.it GmbH.
You may use this code for testing only.
Any commercial use of this license is prohibited.
For a proprietary license for commercial use, please contact info@slock.it.

A more open license is currently under development and will be applied soon.
For information about liability, maintenance etc. also refer to the contract concluded with Slock.it GmbH.


Disclaimer of Warranty.

  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

Limitation of Liability.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

