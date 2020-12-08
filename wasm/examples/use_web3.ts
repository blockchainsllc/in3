/// use IN3 as Web3Provider in web3.js
import {IN3} from 'in3'

const Web3 = require('web3')

const in3 = new IN3({
    proof: 'standard',
    signatureCount: 1,
    requestCount: 1,
    chainId: 'goerli',
    replaceLatestBlock: 10
})

// Use IN3 network client as a Http-Provider
const web3 = new Web3(in3.createWeb3Provider());

(async () => {
    const block = await web3.eth.getBlock('latest')
    console.log("Block : ", block)
})().catch(console.error);
