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


  setKey() {
    return this.send('zksync_setKey')
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