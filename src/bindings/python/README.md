## Incubed Client
Python bindings and library for in3. Go to our [readthedocs](https://in3.readthedocs.io/) page for more on usage.

This library is based on the [C version of Incubed](http://github.com/slockit/in3-c), which limits the compatibility for Cython, so please contribute by compiling it to your own platform and sending us a pull-request!


### Quickstart

##### Install with [pip](https://pip.pypa.io/en/stable/installing/)
```python
pip install in3
```
##### In3 Client Standalone
```python
import in3

in3_client = in3.Client()
block_number = in3_client.eth.block_number()
print(block_number) # Mainnet's block number

in3_client.eth # ethereum module
in3_client.in3 # in3 module 
```

##### Tests
```bash
python tests/test_suite.py
```

##### Contributing
1. Get the latest `libin3.dylib` from the Gitlab Pipeline on the `in-core` project and replace it in `in3/bind` folder. 
2. Run all tests again
3. Fix possible version issues. 