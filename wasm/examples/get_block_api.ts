/// Reads the latest block by calling IN3's web3.js-compatible eth API.
/// Read the eth api from web3.js docs: https://web3js.readthedocs.io/en/v1.3.0/web3-eth.html
import {IN3} from 'in3'

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
