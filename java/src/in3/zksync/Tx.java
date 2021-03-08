package in3.zksync;

import in3.utils.JSON;

/**
 * A ZKSync Tx.
 */
public class Tx {

  private JSON data;

  private Tx(JSON data) {
    this.data = data;
  }

  public static Tx asTx(Object o) {
    if (o == null)
      return null;
    return new Tx((JSON) o);
  }

  /**
   * The Block-number or null.
   */
  public Long getBlock() {
    return data.getLong("block");
  }

  /**
   * true if the tx was executed.
   */
  public boolean isExecuted() {
    return data.getBoolean("executed");
  }

  /**
   * the reason in case the tx failed.
   */
  public String getId() {
    return data.getString("failReason");
  }

  /**
   * if the transaction was executed, this will indicate the success. (or null)
   */
  public Boolean isSuccess() {
    return data.getBoolean("success");
  }
}
