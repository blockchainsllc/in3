/// read block as rpc

const IN3 = require('in3')


async function showLatestBlock() {
    // create new IN3 instance
    var c = IN3({
        chainId: 0x5 // use goerli
    })

    await IN3.setConfig({
        chainId: 0x5 // use goerli
    })

    // send raw RPC-Request (this would throw if the response contains an error)
    const lastBlockResponse = await c.sendRPC('eth_getBlockByNumber', ['latest', false])

    console.log("latest Block: ", JSON.stringify(lastBlockResponse, null, 2))

    // clean up
    c.free()

}

showLatestBlock().catch(console.error)
