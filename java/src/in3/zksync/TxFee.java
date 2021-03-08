package in3.zksync;

import in3.utils.JSON;

/**
 * A ZKSync TxFee.
 */
public class TxFee {

  private JSON data;

  private TxFee(JSON data) {
    this.data = data;
  }

  public static TxFee asTxFee(Object o) {
    if (o == null)
      return null;
    return new TxFee((JSON) o);
  }

  /**
   * the type of fee.
   */
  public String getFeeType() {
    return data.getString("feeType");
  }

  /**
   * The fee paid for gas costs.
   */
  public long getGasFee() {
    return data.getLong("gasFee");
  }

  /**
   * The gas Price in Wei
   */
  public long getGasPriceWei() {
    return data.getLong("gasPriceWei");
  }

  /**
   * The fee for the amount.
   */
  public long getGasTxAmount() {
    return data.getLong("gasTxAmount");
  }
  /**
   * The total amoiunt of fee to be paid for the tx.
   */
  public long getTotalFee() {
    return data.getLong("totalFee");
  }
  /**
   * The zkp fee.
   */
  public long getZkpFee() {
    return data.getLong("zkpFee");
  }
}
