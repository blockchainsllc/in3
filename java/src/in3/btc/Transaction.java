package in3.btc;

import in3.utils.JSON;

/**
 * A BitCoin Transaction.
 */
public class Transaction {

  private JSON data;

  private Transaction(JSON data) {
    this.data = data;
  }

  public static Transaction asTransaction(Object o) {
    if (o == null)
      return null;
    return new Transaction((JSON) o);
  }

  public static Transaction[] asTransactions(Object o) {
    if (o == null)
      return null;
    Object[] a      = (Object[]) o;
    Transaction[] b = new Transaction[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = Transaction.asTransaction(a[i]);
    return b;
  }

  /**
   * Transaction Id.
   */
  public String getTxid() {
    return data.getString("txid");
  }

  /**
   * The transaction hash (differs from txid for witness transactions).
   */
  public String getHash() {
    return data.getString("hash");
  }

  /**
   * The version.
   */
  public long getVersion() {
    return data.getLong("version");
  }

  /**
   * The serialized transaction size.
   */
  public long getSize() {
    return data.getLong("size");
  }

  /**
   * The virtual transaction size (differs from size for witness transactions).
   */
  public long getVsize() {
    return data.getLong("vsize");
  }

  /**
   * The transactions weight (between vsize4-3 and vsize4).
   */
  public long getWeight() {
    return data.getLong("weight");
  }

  /**
   * The locktime.
   */
  public long getLocktime() {
    return data.getLong("locktime");
  }

  /**
   * The hex representation of raw data.
   */
  public String getHex() {
    return data.getString("hex");
  }

  /**
   * The block hash of the block containing this transaction.
   */
  public String getBlockhash() {
    return data.getString("blockhash");
  }

  /**
   * The confirmations.
   */
  public long getConfirmations() {
    return data.getLong("confirmations");
  }

  /**
   * The transaction time in seconds since epoch (Jan 1 1970 GMT).
   */
  public long getTime() {
    return data.getLong("time");
  }

  /**
   * The block time in seconds since epoch (Jan 1 1970 GMT).
   */
  public long getBlocktime() {
    return data.getLong("blocktime");
  }

  /**
   * The transaction inputs.
   */
  public TransactionInput[] getVin() {
    Object[] tx = (Object[]) data.get("vin");
    if (tx == null || tx.length == 0)
      return new TransactionInput[0];

    TransactionInput[] res = new TransactionInput[tx.length];
    for (int i = 0; i < tx.length; i++)
      res[i] = TransactionInput.asTransactionInput(tx[i]);
    return res;
  }

  /**
   * The transaction outputs.
   */
  public TransactionOutput[] getVout() {
    Object[] tx = (Object[]) data.get("vout");
    if (tx == null || tx.length == 0)
      return new TransactionOutput[0];

    TransactionOutput[] res = new TransactionOutput[tx.length];
    for (int i = 0; i < tx.length; i++)
      res[i] = TransactionOutput.asTransactionOutput(tx[i]);
    return res;
  }
}
