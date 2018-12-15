This library contains the core API + management of the nodeList as well as the capability to sign requests

Within the src-folder there are different modules used create the binaries matching the requirements of the device.

```mermaid
graph LR

subgraph exe/lib
     cmd(cmd)
     lib(lib)
 end

subgraph verifier
     eth_full(eth_full)
     eth_basic(eth_basic)
     eth_nano(eth_nano)
 end

subgraph transport
   transport_curl
   transport_ble
   transport_wirepass
 end

subgraph lib
   core((core))
 end


transport_curl -.-> core
transport_curl --> libcurl

transport_ble -.-> core
transport_ble --> zephyr

transport_wirepass -.-> core
transport_wirepass --> wirepass


cmd --> transport_curl
cmd --> eth_full
cmd -.-> core

lib --> transport_curl
lib --> eth_full
lib -.-> core

test --> transport_curl
test --> eth_full
test -.-> core

eth_nano -.-> core

eth_basic -.-> core
eth_basic --> eth_nano

eth_full -.-> core
eth_full --> eth_basic

style libcurl stroke-width:2px,stroke-dasharray: 5, 5

```
