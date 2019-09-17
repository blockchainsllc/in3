package in3;

import in3.IN3;
import in3.eth1.TransactionRequest;

/**
 * a Interface responsible for signing data or transactions.
 */
public interface Signer {
    /**
     * optiional method which allows to change the transaction-data before sending
     * it. This can be used for redirecting it through a multisig.
     */
    TransactionRequest prepareTransaction(IN3 in3, TransactionRequest tx);

    /** returns true if the account is supported (or unlocked) */
    boolean hasAccount(String address);

    /** signing of the raw data. */
    String sign(String data, String address);

}
