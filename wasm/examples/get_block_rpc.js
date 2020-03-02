/// read block as rpc

const IN3 = require('in3-wasm')

async function showLatestBlock() {
    // create new incubed instance
    var c = new IN3()

    await c.setConfig({
        chainId: 0x5 // use goerli
    })

    // send raw RPC-Request
    const lastBlockResponse = await c.send({ method: 'eth_getBlockByNumber', params: ['latest', false] })

    if (lastBlockResponse.error)
        console.error("Error getting the latest block : ", lastBlockResponse.error)
    else
        console.log("latest Block: ", JSON.stringify(lastBlockResponse.result, null, 2))

    // clean up
    c.free()

}

showLatestBlock().catch(console.error)