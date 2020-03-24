# Sample Signature App for Ledger Blue & Ledger Nano S

This application demonstrates a more complex user interface, the Secure Element
proxy logic, cryptographic APIs and flash storage.

Run `make load` to build and load the application onto the device. After
installing and running the application, you can run `demo.py` to test a
signature over USB.

Note that in order to run `demo.py`, you must install the `secp256k1` Python
package:

```
pip install secp256k1
```

See [Ledger's documentation](http://ledger.readthedocs.io) to get started.
