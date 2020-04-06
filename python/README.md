
Python bindings and library for in3. Go to our [readthedocs](https://in3.readthedocs.io/) page for more on usage.

This library is based on the [C version of Incubed](http://github.com/slockit/in3-c), which limits the compatibility for Cython, so please contribute by compiling it to your own platform and sending us a pull-request!


## Quickstart

### Install with pip 
 
```python
pip install in3
```

### In3 Client Standalone

```python
import in3

in3_client = in3.Client()
block_number = in3_client.eth.block_number()
print(block_number) # Mainnet's block number

in3_client.eth # ethereum module
in3_client.in3 # in3 module 
```

### Tests
```bash
python tests/test_suite.py
```

### Contributing
1. Read the index and respect the architecture. For additional packages and files please update the index.
2. (Optional) Get the latest `libin3.dylib` from the Gitlab Pipeline on the `in-core` project and replace it in `in3/bind` folder. 
3. Write the changes in a new branch, then make a pull request for the `develop` branch when all tests are passing. Be sure to add new tests to the CI. 

### Index
Explanation of this source code architecture and how it is organized. For more on design-patterns see [here](http://geekswithblogs.net/joycsharp/archive/2012/02/19/design-patterns-for-model.aspx) or on [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.

- **in3.__init__.py**: Library entry point, imports organization. Standard for any pipy package.
- **in3.eth**: Package for Ethereum objects and tools.
- **in3.eth.account**: Api for managing simple wallets and smart-contracts alike.
- **in3.eth.api**: Ethereum tools and Domain Objects.
- **in3.eth.model**: Value Objects for Ethereum. 
- **in3.libin3**: Package for everything related to binding libin3 to python. Libin3 is written in C and can be found [here](https://github.com/slockit/in3-c).
- **in3.libin3.shared**: Native shared libraries for multiple operating systems and platforms.
- **in3.libin3.enum**: Enumerations mapping C definitions to python.
- **in3.libin3.lib_loader**: Bindings using Ctypes.
- **in3.libin3.runtime**: Runtime object, bridging the remote procedure calls to the libin3 instances. 
