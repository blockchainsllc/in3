package in3.zksync;

import in3.IN3;
import java.math.BigInteger;

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
    return (String) in3.sendRPCasObject("zksync_contract_address", new Object[] {});
  }

  /**
  * the available tokens.
   */
  public Token[] getTokens() {
    return Token.asTokens(in3.sendRPCasObject("zksync_tokens", new Object[] {}));
  }

  /**
  * returns the current balance, nonce and key for the given account. if address is null, the current configured Account will be used.
  */
  public Account getAccountInfo(String address) {
    return Account.asAccount(in3.sendRPCasObject("zksync_account_info", address == null ? new Object[] {} : new Object[] {address}));
  }

  /**
  * the Transaction State.
   */
  public Tx getTransactionInfo(String txid) {
    return Tx.asTx(in3.sendRPCasObject("zksync_tx_info", new Object[] {txid}));
  }

  /**
  * the EthOp State. (State of a deposit or emergencyWithdraw - Transaction )
   */
  public EthOp getEthOpInfo(String txid) {
    return EthOp.asEthOp(in3.sendRPCasObject("zksync_ethop_info", new Object[] {txid}));
  }

  /**
  * sets the sync keys and returns the confirmed pubkeyhash
   */
  public String setKey(String token) {
    return (String) in3.sendRPCasObject("zksync_set_key", new Object[] {token});
  }

  /**
  * returns the pubkeyhash based on the current config.
   */
  public String getPubKeyHash() {
    return (String) in3.sendRPCasObject("zksync_pubkeyhash", new Object[] {});
  }

  /**
  * returns the public key based on the current config.
   */
  public String getPubKey() {
    return (String) in3.sendRPCasObject("zksync_pubkey", new Object[] {});
  }
  /**
  * returns the private key based on the current config.
   */
  public String getSyncKey() {
    return (String) in3.sendRPCasObject("zksync_sync_key", new Object[] {});
  }

  /**
  * returns the address of the account based on the current config.
   */
  public String getAccountAddress() {
    return (String) in3.sendRPCasObject("zksync_account_address", new Object[] {});
  }

  /**
  * signs the data and returns a musig schnorr signature.
   */
  public String sign(byte[] message) {
    return (String) in3.sendRPCasObject("zksync_sign", new Object[] {message});
  }

  /**
  * signs the data and returns a musig schnorr signature.
   */
  public boolean verify(byte[] message, String signature) {
    return (Boolean) in3.sendRPCasObject("zksync_verify", new Object[] {message, signature});
  }

  /**
  * calculates the current tx fees for the specified 
   */
  public TxFee getTxFee(String txType, String toAddress, String token) {
    return TxFee.asTxFee(in3.sendRPCasObject("zksync_get_tx_fee", new Object[] {txType, toAddress, token}));
  }

  /**
  * sends the specified amount of tokens to the zksync contract as deposit for the specified amount (or null if this the same as send)
  * returns the ethopId
   */
  public String deposit(BigInteger amount, String token, boolean approveDepositAmountForERC20, String account) {
    return (String) in3.sendRPCasObject("zksync_deposit", new Object[] {amount, token, approveDepositAmountForERC20, account});
  }

  /**
  * transfers the specified amount of tokens in L2
   * returns the txid
   */
  public String transfer(String toAddress, BigInteger amount, String token, String fromAccount) {
    return (String) in3.sendRPCasObject("zksync_transfer", new Object[] {toAddress, amount, token, fromAccount});
  }

  /**
  * withdraw the specified amount of tokens to L1
   * returns the txid
   */
  public String withdraw(String toAddress, BigInteger amount, String token, String fromAccount) {
    return (String) in3.sendRPCasObject("zksync_withdraw", new Object[] {toAddress, amount, token, fromAccount});
  }

  /**
  * withdraw all tokens from L2 to L1.
   * returns the txId
   */
  public String emergencyWithdraw(String token) {
    return (String) in3.sendRPCasObject("zksync_emergency_withdraw", new Object[] {token});
  }

  /**
  * aggregate the pubKey to one pubKeys for signing together as musig Schnorr signatures.
   */
  public String aggregatePubkey(String[] pubKeys) {
    StringBuilder s = new StringBuilder(pubKeys[0]);
    for (int i = 1; i < pubKeys.length; i++) s.append(pubKeys[i].substring(2));
    return (String) in3.sendRPCasObject("zksync_aggregate_pubkey", new Object[] {s.toString()});
  }
}
