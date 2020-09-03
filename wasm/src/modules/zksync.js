in3w.extensions.push(c => c.zksync = new ZksyncAPI(c))

class ZksyncAPI {
  constructor(client) {
    this.client = client
  }

}