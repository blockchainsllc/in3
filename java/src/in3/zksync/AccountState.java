package in3.zksync;

import in3.utils.JSON;
import java.math.BigInteger;

/**
 * A ZKSync AccountState.
 */
public class AccountState {

  private JSON data;

  private AccountState(JSON data) {
    this.data = data;
  }

  public static AccountState asAccountState(Object o) {
    if (o == null)
      return null;
    return new AccountState((JSON) o);
  }

  /**
   * The nonce or TransactionCount.
   */
  public Integer getNonce() {
    return data.getInteger("nonce");
  }

  /**
   * the assigned pubkeyhash.
   */
  public String getPubKeyHash() {
    return data.getString("pubKeyHash");
  }

  /**
   * the balance of the specified token (or null)
   */
  public BigInteger getBalance(String token) {
    return data.getBigInteger(token);
  }
}
