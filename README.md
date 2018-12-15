This library contains the core API + management of the nodeList as well as the capability to sign requests

Within the src-folder there are different modules used create the binaries matching the requirements of the device.

```mermaid
graph LR

subgraph lib
   core((core))
 end
subgraph exe/lib
     cmd(cmd)
     lib(lib)
 end

subgraph verifier
     eth_full(eth_full)
     eth_basic(eth_basic)
     eth_nano(eth_nano)
     ipfs(ipfs)
 end

subgraph transport
   transport_curl
   transport_ble
   transport_wirepass
 end
 
test --> transport_curl
test --> eth_full
test -.-> core



transport_curl --> libcurl

transport_ble --> zephyr

transport_wirepass --> wirepass


cmd --> transport_curl
cmd --> eth_full
cmd -.-> core
cmd --> ipfs

lib --> transport_curl
lib --> eth_full
lib -.-> core
lib --> ipfs


eth_basic --> eth_nano

eth_full --> eth_basic

style libcurl stroke-width:2px,stroke-dasharray: 5, 5

```
