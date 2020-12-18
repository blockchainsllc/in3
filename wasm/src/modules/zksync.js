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
in3w.extensions.push(c => c.zksync = new ZksyncAPI(c))

class ZksyncAPI {
  constructor(client) {
    this.client = client
  }

  send(name, ...params) {
    return this.client.sendRPC(name, params || [])
  }


  getAccountInfo(account) {
    return this.send('zksync_account_info', ...(account ? [account] : []))
  }

  getContractAddress() {
    return this.send('zksync_contract_address')
  }


  getTokens() {
    return this.send('zksync_tokens')
  }


  getTxInfo(txHash) {
    return this.send('zksync_tx_info', txHash)
  }


  setKey(tokenSymbol) {
    return this.send('zksync_setKey', tokenSymbol || 'ETH')
  }


  getEthopInfo(opId) {
    return this.send('zksync_ethop_info', opId)
  }


  getTokenPrice(tokenSymbol) {
    return this.send('zksync_get_token_price', tokenSymbol)
  }


  getTxFee(txType, receipient, token) {
    return this.send('zksync_get_tx_fee', txType, receipient, token)
  }


  getSyncKey() {
    return this.send('zksync_syncKey')
  }


  deposit(amount, token, approveDepositAmountForERC20, account) {
    if (account)
      return this.send('zksync_deposit', amount, token, approveDepositAmountForERC20, account)
    else
      return this.send('zksync_deposit', amount, token, approveDepositAmountForERC20)
  }


  transfer(to, amount, token, account) {
    if (account)
      return this.send('zksync_transfer', to, amount, token, account)
    else
      return this.send('zksync_transfer', to, amount, token)
  }


  withdraw(ethAddress, amount, token, account) {
    if (account)
      return this.send('zksync_withdraw', ethAddress, amount, token, account)
    else
      return this.send('zksync_withdraw', ethAddress, amount, token)
  }


  emergencyWithdraw(token) {
    return this.send('zksync_emergencyWithdraw', token)
  }

}
