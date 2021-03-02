package in3.zksync;

import in3.utils.JSON;

/**
 * A ZKSync Account.
 */
public class Account {

  private JSON data;

  private Account(JSON data) {
    this.data = data;
  }

  public static Account asAccount(Object o) {
    if (o == null)
      return null;
    return new Account((JSON) o);
  }

  public static Account[] asAccounts(Object o) {
    if (o == null)
      return null;
    Object[] a  = (Object[]) o;
    Account[] b = new Account[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = Account.asAccount(a[i]);
    return b;
  }

  /**
   * The address of the Account.
   */
  public String getAddress() {
    return data.getString("address");
  }

  /**
   * id or null if it is not assigned yet.
   */
  public Integer getId() {
    return data.getInteger("id");
  }

  /**
   * the commited state
   */
  public AccountState getCommited() {
    return AccountState.asAccountState(data.getObject("commited"));
  }

  /**
   * the pending depositing  state
   */
  public AccountState getDepositing() {
    return AccountState.asAccountState(data.getObject("depositing"));
  }

  /**
   * the verified state
   */
  public AccountState getVerified() {
    return AccountState.asAccountState(data.getObject("verified"));
  }
}
