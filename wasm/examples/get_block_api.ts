/// read block with API

import IN3 from 'in3'

async function showLatestBlock() {

  // create new IN3 instance
  const client = new IN3({
    proof              : 'standard',
    signatureCount     : 1,
    chainId            : 'goerli'
  })

  const lastBlock = await client.eth.getBlockByNumber()

  console.log("latest Block: ", JSON.stringify(lastBlock, null, 2))

}

showLatestBlock().catch(console.error)
