package in3.zksync;

import in3.utils.JSON;

/**
 * A ZKSync EthOperation.
 */
public class EthOp {

  private JSON data;

  private EthOp(JSON data) {
    this.data = data;
  }

  public static EthOp asEthOp(Object o) {
    if (o == null)
      return null;
    return new EthOp((JSON) o);
  }

  /**
   * returns true, if the operation was executed
   */
  public boolean isExecuted() {
    return data.getBoolean("executed");
  }

  /**
   * returns the BlockNumber or null if it was not executed.
   */
  public Long getBlockNumber() {
    JSON block = (JSON) data.getObject("block");
    return block == null ? null : data.getLong("blockNumber");
  }

  /**
   * returns true if the operation is commited in L2
   */
  public boolean isCommitted() {
    JSON block = (JSON) data.getObject("block");
    return block == null ? false : data.getBoolean("commited");
  }

  /**
   * returns true if the operation is verified in L1
   */
  public boolean isVerified() {
    JSON block = (JSON) data.getObject("block");
    return block == null ? false : data.getBoolean("verified");
  }
}
