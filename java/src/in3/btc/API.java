package in3.btc;

import in3.IN3;

/**
 * API for handling BitCoin data. Use it when connected to Chain.BTC.
 */
public class API {
  private IN3 in3;

  private static final String BTC_GET_BLOCK           = "getblock";
  private static final String BTC_GET_BLOCKHEADER     = "getblockheader";
  private static final String BTC_GET_RAW_TRANSACTION = "getrawtransaction";

  /**
   * creates a btc.API using the given incubed instance.
   */
  public API(IN3 in3) {
    this.in3 = in3;
  }

  /**
   * Retrieves the transaction and returns the data as json.
   */
  public Transaction getTransaction(String txid) {
    return Transaction.asTransaction(in3.sendRPCasObject(BTC_GET_RAW_TRANSACTION, new Object[] {txid}));
  }

  /**
   * Retrieves the serialized transaction (bytes).
   */
  public byte[] getTransactionBytes(String txid) {
    Object jsonResponse = in3.sendRPCasObject(BTC_GET_RAW_TRANSACTION, new Object[] {txid, false});
    return jsonResponse == null ? new byte[] {} : decodeHexString((String) jsonResponse);
  }

  /**
   * Retrieves the blockheader.
   */
  public BlockHeader getBlockHeader(String blockHash) {
    return BlockHeader.asBlockHeader(in3.sendRPCasObject(BTC_GET_BLOCKHEADER, new Object[] {blockHash, true}));
  }

  /**
   * Retrieves the byte array representing teh serialized blockheader data.
   */
  public byte[] getBlockHeaderBytes(String blockHash) {
    Object jsonResponse = in3.sendRPCasObject(BTC_GET_BLOCKHEADER, new Object[] {blockHash, false});
    return jsonResponse == null ? new byte[] {} : decodeHexString((String) jsonResponse);
  }

  /**
   * Retrieves the block including the full transaction data. Use
   * Api#GetBlockWithTxIds" for only the transaction ids.
   */
  public Block getBlockWithTxData(String blockHash) {
    return Block.asBlock(in3.sendRPCasObject(BTC_GET_BLOCK, new Object[] {blockHash, 2}));
  }

  /**
   * Retrieves the block including only transaction ids. Use
   * Api#GetBlockWithTxData for the full transaction data.
   */
  public Block getBlockWithTxIds(String blockHash) {
    return Block.asBlock(in3.sendRPCasObject(BTC_GET_BLOCK, new Object[] {blockHash, 1}));
  }

  /**
   * Retrieves the serialized block in bytes.
   */
  public byte[] getBlockBytes(String blockHash) {
    Object jsonResponse = in3.sendRPCasObject(BTC_GET_BLOCK, new Object[] {blockHash, false});
    return jsonResponse == null ? new byte[] {} : decodeHexString((String) jsonResponse);
  }

  // Copied from https://www.baeldung.com/java-byte-arrays-hex-strings, consider
  // extracting this to an utility package.
  private byte[] decodeHexString(String hexString) {
    if (hexString.length() % 2 == 1) {
      throw new IllegalArgumentException("Invalid hexadecimal String supplied.");
    }

    byte[] bytes = new byte[hexString.length() / 2];
    for (int i = 0; i < hexString.length(); i += 2) {
      bytes[i / 2] = hexToByte(hexString.substring(i, i + 2));
    }
    return bytes;
  }

  private byte hexToByte(String hexString) {
    int firstDigit  = toDigit(hexString.charAt(0));
    int secondDigit = toDigit(hexString.charAt(1));
    return (byte) ((firstDigit << 4) + secondDigit);
  }

  private int toDigit(char hexChar) {
    int digit = Character.digit(hexChar, 16);
    if (digit == -1) {
      throw new IllegalArgumentException("Invalid Hexadecimal Character: " + hexChar);
    }
    return digit;
  }
}
