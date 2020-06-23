package in3.btc;

import in3.utils.JSON;

/**
 * Input of a transaction.
 */
public class TransactionInput {
  private JSON data;

  private TransactionInput(JSON data) {
    this.data = data;
  }

  protected static TransactionInput asTransactionInput(Object o) {
    if (o == null)
      return null;
    return new TransactionInput((JSON) o);
  }

  protected static TransactionInput[] asTransactionInputs(Object o) {
    if (o == null)
      return null;
    Object[] a           = (Object[]) o;
    TransactionInput[] b = new TransactionInput[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = TransactionInput.asTransactionInput(a[i]);
    return b;
  }

  /**
   * The transaction id.
   */
  public String getTxid() {
    return data.getString("txid");
  }

  /**
   * The index of the transactionoutput.
   */
  public long getYout() {
    return data.getLong("vout");
  }

  /**
   * The script.
   */
  public ScriptSig getScriptSig() {
    Object tx = (Object) data.get("scriptSig");
    if (tx == null)
      return null;

    return ScriptSig.asScriptSig(tx);
  }

  /**
   * Hex-encoded witness data (if any).
   */
  public String[] getTxinwitness() {
    return data.getStringArray("txinwitness");
  }

  /**
   * The script sequence number.
   */
  public long getSequence() {
    return data.getLong("sequence");
  }
}
