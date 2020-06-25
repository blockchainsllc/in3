package in3.btc;

import in3.utils.JSON;

/**
 * A BitCoin Transaction.
 */
public class TransactionOutput {
  private JSON data;

  public TransactionOutput(JSON data) {
    this.data = data;
  }

  protected static TransactionOutput asTransactionOutput(Object o) {
    if (o == null)
      return null;
    return new TransactionOutput((JSON) o);
  }

  protected static TransactionOutput[] asTransactionOutputs(Object o) {
    if (o == null)
      return null;
    Object[] a            = (Object[]) o;
    TransactionOutput[] b = new TransactionOutput[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = TransactionOutput.asTransactionOutput(a[i]);
    return b;
  }

  /**
   * The value in bitcoins.
   */
  public float getValue() {
    String value = data.getString("value");
    return Float.parseFloat(value);
  }

  /**
   * The index in the transaction.
   */
  public long getN() {
    return data.getLong("n");
  }

  /**
   * The script of the transaction.
   */
  public ScriptPubKey getScriptPubKey() {
    Object tx = (Object) data.get("scriptPubKey");
    if (tx == null)
      return null;

    return ScriptPubKey.asScriptPubKey(tx);
  }
}
