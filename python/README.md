## Python Incubed client
![coverage badge](docs/coverage.svg)

This library is based on the [C version of Incubed](http://github.com/slockit/in3-c), which limits the compatibility for Cython, so please contribute by compiling it to your own platform and sending us a pull-request!

Go to our [readthedocs](https://in3.readthedocs.io/) page for more.

### Quickstart

#### Install with pip 
 
```shell script
coverage run -m pytest --pylama --junitxml=report.xml && coverage report && coverage-badge -fo docs/coverage.svg
```

#### In3 Client API

```python
import in3

in3_client = in3.Client()
# Sends a request to the Incubed Network, that in turn will collect proofs from the Ethereum client, 
# attest and sign the response, then send back to the client, that will verify signatures and proofs. 
block_number = in3_client.eth.block_number()
print(block_number) # Mainnet's block number

in3_client  # incubed network api 
in3_client.eth  # ethereum api
in3_client.account  # ethereum account api
in3_client.contract  # ethereum smart-contract api
```

#### Tests
```bash
pytest --pylama
```

#### Index
Explanation of this source code architecture and how it is organized. For more on design-patterns see [here](http://geekswithblogs.net/joycsharp/archive/2012/02/19/design-patterns-for-model.aspx) or on [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.

- **in3.__init__.py**: Library entry point, imports organization. Standard for any pipy package.
- **in3.client**: Incubed Client and API.
- **in3.model**: MVC Model classes for the Incubed client module domain.
- **in3.transport**: HTTP Transport function and error handling.
- **in3.wallet**: WiP - Wallet API.
- **in3.exception**: Custom exceptions. 
- **in3.eth**: Ethereum module.
- **in3.eth.api**: Ethereum API.
- **in3.eth.account**: Ethereum accounts.
- **in3.eth.contract**: Ethereum smart-contracts API.
- **in3.eth.model**: MVC Model classes for the Ethereum client module domain. Manages serializaation.
- **in3.eth.factory**: Ethereum Object Factory. Manages deserialization.
- **in3.libin3**: Module for the libin3 runtime. Libin3 is written in C and can be found [here](https://github.com/slockit/in3-c).
- **in3.libin3.shared**: Native shared libraries for multiple operating systems and platforms.
- **in3.libin3.enum**: Enumerations mapping C definitions to python.
- **in3.libin3.lib_loader**: Bindings using Ctypes.
- **in3.libin3.runtime**: Runtime object, bridging the remote procedure calls to the libin3 instances. 

