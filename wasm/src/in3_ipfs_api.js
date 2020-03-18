class IpfsAPI {
  constructor(client) {
    this.client = client
    this.encoding = 'base64'
  }

  /**
   * retrieves the content for a hash from IPFS.
   * @param multihash  the IPFS-hash to fetch
   *
   */
  get(multihash) {
    return this.client.sendRPC('ipfs_get', [multihash, this.encoding])
      .then(response => response || Promise.reject(new Error(response.error || 'Hash not found')))
      .then(result => this.client.util.base64Decode(result))
      .then(result => this.client.util.toBuffer(result))
  }

  /**
   * stores the data on ipfs and returns the IPFS-Hash.
   * @param content puts a IPFS content
   */
  put(data) {
    let encoded = this.client.util.base64Encode(this.client.util.toBuffer(data))
    return this.client.sendRPC('ipfs_put', [encoded, this.encoding])
      .then(response => response || Promise.reject(new Error(response.error || 'Hash not found')))
  }
}