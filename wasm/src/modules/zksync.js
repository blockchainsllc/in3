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

}