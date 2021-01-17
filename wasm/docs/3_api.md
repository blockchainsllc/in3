## Incubed Module

This page contains a list of all Datastructures and Classes used within the IN3 WASM-Client

Importing incubed is as easy as 
```ts
import {IN3} from "in3-wasm"
```


### BufferType and BigIntType

The WASM-Module comes with no dependencies. This means per default it uses the standard classes provided as part of the EMCAScript-Standard.

If you work with a library which expects different types, you can change the generic-type and giving a converter:

#### Type BigIntType

Per default we use `bigint`. This is used whenever we work with number too big to be stored as a `number`-type.

If you want to change this type, use [setConverBigInt()](#setconvertbigint) function.

#### Type Buffer

Per default we use `UInt8Array`. This is used whenever we work with raw bytes.

If you want to change this type, use [setConverBuffer()](#setconvertbuffer) function.

#### Generics

```js
import {IN3Generic} from 'in3-wasm'
import BN from 'bn.js'

// create a new client by setting the Generic Types
const c = new IN3Generic<BN,Buffer>()

// set the converter-functions
IN3Generic.setConverBuffer(val => Buffer.from(val))
IN3Generic.setConverBigInt(val => new BN(val))

```



### Package

While the In3Client-class is also the default import, the following imports can be used:

