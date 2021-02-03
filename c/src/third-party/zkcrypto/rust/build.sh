# generate C bindings
which cbindgen || cargo install cbindgen
cbindgen --config cbindgen.toml --crate zk-crypto --output zkcrypto.h
cargo build --release 


# generate WASM bindings
which wasm-pack || cargo install wasm-pack
wasm-pack build --release --target=nodejs --out-name=musig-bindings --out-dir=musig-bindings
