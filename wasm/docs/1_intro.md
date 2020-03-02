# API Reference WASM

Even though the incubed client is written in C, we are using emscripten to build wasm. Together with some binding-code incubed runs in any Javascript-Runtime.
Using WASM gives us 3 important features:

1. Performance. 
   Since WASM runs at almost native speed it is very fast

2. Security
   Since the WASM-Module has no dependencies it reduces the risk of using a malicious dependency, which would be able to manipulate Prototypes.
   Also, since the real work is happening inside the wasm, trying to change Prototype would not work.

3. Size
   The current wasm-file is about 200kb. This is smaller then most other libraries and can easily be used in any app or website.


## Installing

This client uses the in3-core sources compiled to wasm. The wasm is included into the js-file wich makes it easier to include the data.
This module has **no** dependencies! All it needs is included inta a wasm of about 300kB.

Installing incubed is as easy as installing any other module:

```
npm install --save in3-wasm
```

### WASM-support

Even though most browsers and javascript enviroment such as nodejs, have full support for wasm, there are ocasions, where WASM is fully supported.
In case you want to run incubed within a react native app, you might face such issues. In this case you can use [in3-asmjs](https://www.npmjs.com/package/in3-asmjs), which has the same API, but runs on pure javascript (a bit slower and bigger, but full support everywhere).



