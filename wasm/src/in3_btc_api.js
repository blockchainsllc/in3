class BtcAPI {
  constructor(client) {
    this.client = client
  }

  getTransaction(txid) {
    return this.client.sendRPC('getrawtransaction', [txid, true])
      .then(response => response || Promise.reject(new Error(response.error || 'txid not found')))
  }

  getTransactionData(txid) {
    return this.client.sendRPC('getrawtransaction', [txid, false])
      .then(response => response || Promise.reject(new Error(response.error || 'txid not found')))
      .then(result => this.client.util.toBuffer('0x' + result))
  }

}