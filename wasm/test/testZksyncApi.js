/*******************************************************************************
 * This file is part of the IN3 project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2021 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/
require('mocha')
const {assert} = require('chai')
const {hasAPI, createClient, mockResponse, IN3, beforeTest} = require('./util/mocker')

if (hasAPI("zksync"))
  describe("ZkSyncApi-Tests", () => {

    beforeEach(beforeTest)
    afterEach(IN3.freeAll)

    it("zksync.getContractAddress", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("contract_address", "test1")

      let contract = await c.zksync.getContractAddress()

      assert.equal(contract.govContract, "0x15C6a183507307986A9682951d354DCE7EB1a2f1")
      assert.equal(contract.mainContract, "0x90cF9aF56cA57610225cC79abB01660bCF6c4c80")
    })

    it("zksync.getTokens", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("tokens", "test1")

      let tokens = await c.zksync.getTokens()

      assert.hasAllKeys(tokens, ["BAT", "DAI", "ETH", "wBTC"])
      assert.hasAllKeys(tokens.BAT, ["address", "decimals", "id", "symbol"])
      assert.hasAllKeys(tokens.DAI, ["address", "decimals", "id", "symbol"])
      assert.hasAllKeys(tokens.ETH, ["address", "decimals", "id", "symbol"])
      assert.hasAllKeys(tokens.wBTC, ["address", "decimals", "id", "symbol"])
      assert.equal(tokens.BAT.address, "0x47b373c49d10ed9e76ab45c4d6a2248854eb7c72")
      assert.equal(tokens.BAT.decimals, 18)
      assert.equal(tokens.BAT.id, 3)
      assert.equal(tokens.BAT.symbol, "BAT")
      assert.equal(tokens.DAI.address, "0x245159a88291931dfdebcd20833533f2e05fa5ef")
      assert.equal(tokens.DAI.decimals, 18)
      assert.equal(tokens.DAI.id, 1)
      assert.equal(tokens.DAI.symbol, "DAI")
      assert.equal(tokens.ETH.address, "0x0000000000000000000000000000000000000000")
      assert.equal(tokens.ETH.decimals, 18)
      assert.equal(tokens.ETH.id, 0)
      assert.equal(tokens.ETH.symbol, "ETH")
      assert.equal(tokens.wBTC.address, "0x6296e99a56bc92ff200fbfab1ecca899366e1014")
      assert.equal(tokens.wBTC.decimals, 8)
      assert.equal(tokens.wBTC.id, 2)
      assert.equal(tokens.wBTC.symbol, "wBTC")
    })

    //TODO: Real data result needed
    it("zksync.getTxInfo", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("tx_info", "test1")

      let txInfo = await c.zksync.getTxInfo("sync-tx:68f9c0e845cc860d754c788ec8c6e15c4269a23c2e665bfdd4d5d4c463402ab9")

      assert.hasAllKeys(txInfo, ["block", "executed", "failReason", "success"])
      assert.isNull(txInfo.block)
      assert.isNull(txInfo.failReason)
      assert.isNull(txInfo.success)

    })

    //TODO: setKey
    it("zksync.setKey", async () => {

      const pk = '0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8'
      const address = "0x8A91DC2D28b689474298D91899f0c1baF62cB85b"

      const c = createClient({
        proof: 'none',
        pk: pk,
        debug: true,
        zksync: {
          provider_url: "http://localhost:3030",
          account: address,
          main_contract: "0xa17D5e05dF6d7E4A630d34761d4562445Dc090c3",
          gov_contract: "0xe6e2E5494889FabEbb418aAD65440B253EB64970"

        },
        rpc: 'http://localhost:8545',
        chainId: 0x11
      })

      mockResponse("account_info", "setKey1")
      mockResponse("tx_submit", "setKey1")
      //c.signer = new IN3.SimpleSigner("0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8")
      //mockResponse("", "test1")

      let contract = await c.zksync.setKey()

    })

    it("zksync.getEthopInfo", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("ethop_info", "test1")

      let info = await c.zksync.getEthopInfo(0)

      assert.hasAllKeys(info, ["block", "executed"])
      assert.hasAllKeys(info.block, ["blockNumber", "committed", "verified"])
      assert.equal(info.block.blockNumber, 1)
      assert.isTrue(info.block.committed)
      assert.isTrue(info.block.verified)
      assert.isTrue(info.executed)

    })

    it("zksync.getTokenPrice", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("get_token_price", "test1")

      let price = await c.zksync.getTokenPrice("wBTC")

      assert.equal(price, "9717.320635")

    })

    it("zksync.getTxFee", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("get_tx_fee", "test1")

      let fee = await c.zksync.getTxFee("Withdraw", "0x8A91DC2D28b689474298D91899f0c1baF62cB85b", "ETH")

      assert.hasAllKeys(fee, ["feeType", "gasFee", "gasPriceWei", "gasTxAmount", "totalFee", "zkpFee"])
      assert.equal(fee.feeType, "Withdraw")
      assert.equal(fee.gasFee, "4041393750000000")
      assert.equal(fee.gasPriceWei, "44904375000")
      assert.equal(fee.gasTxAmount, "90000")
      assert.equal(fee.totalFee, "4070000000000000")
      assert.equal(fee.zkpFee, "32489422687419")
    })

    //yeah no clue mate
    it("zksync.getSyncKey", async () => {

      const c = createClient({
        proof: 'none',
        zksync: {
          provider_url: "http://localhost:3030",
          account: ""
        },
      })

      mockResponse("account_info", "setkey1")
      mockResponse("tx_submit", "setkey1")

      let key = await c.zksync.getSyncKey()
      console.log(key)

    })

    it("zksync.deposit", async () => {

      const pk = '0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8'
      const address = "0x8A91DC2D28b689474298D91899f0c1baF62cB85b"

      const c = createClient({
        proof: 'none',
        pk: pk,
        debug: true,
        zksync: {
          provider_url: "http://localhost:3030",
          account: address,
          main_contract: "0xa17D5e05dF6d7E4A630d34761d4562445Dc090c3",
          gov_contract: "0xe6e2E5494889FabEbb418aAD65440B253EB64970"

        },
        rpc: 'http://localhost:8545',
        chainId: 0x11
      })

      //c.signer = new IN3.SimpleSigner(pk)

      mockResponse("deposit", "test1")
      mockResponse("tokens", "test1")
      mockResponse("eth_chainId", "zksync_deposit")
      mockResponse("eth_getTransactionCount", "zksync_deposit")
      mockResponse("eth_gasPrice", "zksync_deposit")
      mockResponse("eth_sendRawTransaction", "zksync_deposit")
      mockResponse("eth_getTransactionReceipt", "zksync_deposit")


      let receipt = await c.zksync.deposit(100000, "ETH", false)
    //console.log(receipt)
    })


    it("zksync.transfer", async () => {

      const pk = '0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8'
      const address = "0x8A91DC2D28b689474298D91899f0c1baF62cB85b"

      const c = createClient({
        proof: 'none',
        pk: pk,
        debug: true,
        zksync: {
          provider_url: "http://localhost:3030",
          account: address,
          main_contract: "0x0788B765aD1D79f8DFd516C457ab8971Fa0cb003",
          gov_contract: "0xdc7Acd51310F0fA79E1c9Ce99D550a43157c6473"

        },
        rpc: 'http://localhost:8545',
        chainId: 0x11
      })

      mockResponse("account_info", "transfer1")
      mockResponse("tx_submit", "transfer1")
      mockResponse("tokens", "transfer1")
      mockResponse("get_tx_fee", "transfer1")

      let receipt = await c.zksync.transfer("0x60cCA2a21bE53153BD68aB21e835AD739A94fAbD", "1", "ETH")
      console.log(receipt)

    })


    it("zksync.withdraw", async () => {

      const pk = '0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8'
      const address = "0x8A91DC2D28b689474298D91899f0c1baF62cB85b"

      const c = createClient({
        proof: 'none',
        pk: pk,
        debug: true,
        zksync: {
          provider_url: "http://localhost:3030",
          account: address,
          main_contract: "0xfbE0725D462afa7EE6aEedf2b9E5ab8e499fa7cB",
          gov_contract: "0x7883903b084b6e374C060D7980f92CAF87f3d158"

        },
        rpc: 'http://localhost:8545',
        chainId: 0x11
      })


      mockResponse("account_info", "withdraw1")
      mockResponse("tokens", "withdraw1")
      mockResponse("get_tx_fee", "withdraw1")
      mockResponse("tx_submit", "withdraw1")

      let response = await c.zksync.withdraw("0x8A91DC2D28b689474298D91899f0c1baF62cB85b", "10", "ETH")
      console.log(response)

    })


    it("zksync.emergencyWithdraw", async () => {

      const pk = '0xe20eb92b34a3c5bd2ef0802a4bc443a90e73fc4a0edc4781446d7b22a44cc5d8'
      const address = "0x8A91DC2D28b689474298D91899f0c1baF62cB85b"

      const c = createClient({
        proof: 'none',
        pk: pk,
        debug: true,
        zksync: {
          provider_url: "http://localhost:3030",
          account: address,
          main_contract: "0xfbE0725D462afa7EE6aEedf2b9E5ab8e499fa7cB",
          gov_contract: "0x7883903b084b6e374C060D7980f92CAF87f3d158"
        },
        rpc: 'http://localhost:8545',
        chainId: 0x11
      })

      //c.signer = new IN3.SimpleSigner(pk)

      mockResponse("tokens", "emergencyWithdraw1")
      mockResponse("eth_getTransactionCount", "emergencyWithdraw1")
      mockResponse("eth_gasPrice", "emergencyWithdraw1")
      mockResponse("eth_chainId", "emergencyWithdraw1")
      mockResponse("account_info", "emergencyWithdraw1")
      mockResponse("eth_sendRawTransaction", "emergencyWithdraw1")
      mockResponse("eth_getTransactionReceipt", "emergencyWithdraw1")

      let deposit = await c.zksync.emergencyWithdraw("ETH")
      console.log(deposit)

    })
  })
