package in3.btc;

import in3.eth1.Transaction;
import in3.utils.JSON;

/**
 * A Block.
 */
public class Block extends BlockHeader {

  private JSON data;

  private Block(JSON data) {
    super(data);
    this.data = data;
  }

  protected static Block asBlock(Object o) {
    if (o == null)
      return null;
    return new Block((JSON) o);
  }

  protected static Block[] asBlocks(Object o) {
    if (o == null)
      return null;
    Object[] a = (Object[]) o;
    Block[] b  = new Block[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = Block.asBlock(a[i]);
    return b;
  }

  /**
   * Transactions or Transaction of a block.
   */
  public Transaction[] getTransactions() throws Exception {
    Object[] tx = (Object[]) data.get("tx");
    if (tx == null || tx.length == 0)
      return new Transaction[0];

    if (tx[0] instanceof String)
      throw new Exception("The Block only contains the transaction hashes!");

    Transaction[] res = new Transaction[tx.length];
    for (int i = 0; i < tx.length; i++)
      res[i] = Transaction.asTransaction(tx[i]);
    return res;
  }

  /**
   * Transactions or Transaction ids of a block.
   */
  public String[] getTransactionHashes() {
    Object[] tx = (Object[]) data.get("tx");
    if (tx == null || tx.length == 0)
      return new String[0];
    if (tx[0] instanceof String)
      return data.getStringArray("tx");
    String[] res = new String[tx.length];
    for (int i = 0; i < tx.length; i++)
      res[i] = ((JSON) tx[i]).getString("hash");
    return res;
  }

  /**
   * Size of this block in bytes.
   */
  public long getSize() {
    return data.getLong("size");
  }

  /**
   * Weight of this block in bytes.
   */
  public long getWeight() {
    return data.getLong("weight");
  }
}
