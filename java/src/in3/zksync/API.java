package in3.zksync;

import in3.IN3;

/**
 * API for handling ZKSYNC transactions.
 */
public class API {
  private IN3 in3;

  /**
   * creates a zksync.API using the given incubed instance.
   */
  public API(IN3 in3) {
    this.in3 = in3;
  }

  /**
  * the address of the zksync contract.
   */
  public String getContractAddress() {
    return (String) in3.sendRPCasObject("zk_contract_address", new Object[] {});
  }

  /**
  * the available tokens.
   */
  public Token[] getTokens() {
    return Token.asTokens(in3.sendRPCasObject("zk_token", new Object[] {}));
  }

  /**
  * returns the current balance, nonce and key for the given account. if address is null, the current configured Account will be used.
  */
  public Account getAccountInfo(String address) {
    return Account.asAccount(in3.sendRPCasObject("zk_account_info", address == null ? new Object[] {} : new Object[] {address}));
  }

  /**
  * the Transaction State.
   */
  public Tx getTransactionInfos(String txid) {
    return Tx.asTx(in3.sendRPCasObject("zk_tx_info", new Object[] {txid}));
  }
}
