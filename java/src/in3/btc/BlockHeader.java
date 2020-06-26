package in3.btc;

import in3.utils.JSON;

/**
 * A Block header.
 */
public class BlockHeader {

  private JSON data;

  protected BlockHeader(JSON data) {
    this.data = data;
  }

  protected static BlockHeader asBlockHeader(Object o) {
    if (o == null)
      return null;
    return new BlockHeader((JSON) o);
  }

  protected static BlockHeader[] asBlockHeaders(Object o) {
    if (o == null)
      return null;
    Object[] a      = (Object[]) o;
    BlockHeader[] b = new BlockHeader[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = BlockHeader.asBlockHeader(a[i]);
    return b;
  }

  /**
   * The hash of the blockheader.
   */
  public String getHash() {
    return data.getString("hash");
  }

  /**
   * Number of confirmations or blocks mined on top of the containing block.
   */
  public long getConfirmations() {
    return data.getLong("confirmations");
  }

  /**
   * Block number.
   */
  public long getHeight() {
    return data.getLong("height");
  }

  /**
   * Used version.
   */
  public long getVersion() {
    return data.getLong("version");
  }

  /**
   * Version as hex.
   */
  public String getVersionHex() {
    return data.getString("versionHex");
  }

  /**
   * Merkle root of the trie of all transactions in the block.
   */
  public String getMerkleroot() {
    return data.getString("merkleroot");
  }

  /**
   * Unix timestamp in seconds since 1970.
   */
  public long getTime() {
    return data.getLong("time");
  }

  /**
   * Unix timestamp in seconds since 1970.
   */
  public long getMediantime() {
    return data.getLong("mediantime");
  }

  /**
   * Nonce-field of the block.
   */
  public long getNonce() {
    return data.getLong("nonce");
  }

  /**
   * Bits (target) for the block as hex.
   */
  public String getBits() {
    return data.getString("bits");
  }

  /**
   * Difficulty of the block.
   */
  public float getDifficulty() {
    String floatStr = data.getString("difficulty");
    return Float.parseFloat(floatStr);
  }

  /**
   * Total amount of work since genesis.
   */
  public String getChainwork() {
    return data.getString("chainwork");
  }

  /**
   * Number of transactions in the block.
   */
  public long getNTx() {
    return data.getLong("nTx");
  }

  /**
   * Hash of the parent blockheader.
   */
  public String getPreviousblockhash() {
    return data.getString("previousblockhash");
  }

  /**
   * Hash of the next blockheader.
   */
  public String getNextblockhash() {
    return data.getString("nextblockhash");
  }
}
