/// Reads the latest block by calling IN3's internal RPC to the WASM core.
/// Learn other exclusive IN3 RPC calls here: https://in3.readthedocs.io/en/develop/rpc.html
import {IN3} from 'in3'

async function showLatestBlock() {

    // create new IN3 instance
    var c = IN3({
        chainId: 0x5 // use goerli
    })

    // make a RPC (this would throw if the response contains an error)
    const lastBlockResponse = await c.sendRPC('eth_getBlockByNumber', ['latest', false])

    console.log("latest Block: ", JSON.stringify(lastBlockResponse, null, 2))

}

showLatestBlock().catch(console.error)
