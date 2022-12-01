#### nanopb

a simple protobuf implementation, which is used when calculating the hash for IPFS

- version: 0.4.0
- website: https://github.com/nanopb/nanopb
- download: https://github.com/nanopb/nanopb/releases/tag/0.4.0
- changes:
  - `ipfs.pb.{c,h}` were generated using the python generator script (`generator/protoc`)
  - generating these files requires protobuf python lib to be installed on host
  - modified includes to use quotes instead of angular brackets
  - source otherwise unchanged

