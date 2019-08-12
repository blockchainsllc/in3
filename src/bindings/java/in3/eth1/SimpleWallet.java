package in3.eth1;

import java.util.HashMap;
import java.util.Map;

import in3.IN3;
import in3.Signer;

/**
 * a simple Implementation for holding private keys to sing data or
 * transactions.
 */
public class SimpleWallet implements Signer {

    // ke for holding the map
    Map<String, String> privateKeys = new HashMap<String, String>();

    /**
     * adds a key to the wallet and returns its public address.
     */
    public String addRawKey(String data) {
        String address = getAddressFromKey(data);
        // create address
        privateKeys.put(address.toLowerCase(), data);
        return address;
    }

    /**
     * adds a key to the wallet and returns its public address.
     */
    public String addKeyStore(String jsonData, String passphrase) {
        String data = decodeKeystore(jsonData, passphrase);
        if (data == null)
            throw new RuntimeException("Invalid password");
        // create address
        return addRawKey(data);
    }

    /**
     * optiional method which allows to change the transaction-data before sending
     * it. This can be used for redirecting it through a multisig.
     */
    public TransactionRequest prepareTransaction(IN3 in3, TransactionRequest tx) {
        // TODO here you could transform the data in order to support multisigs.
        // for now we don't manipulate the data.
        return tx;
    }

    /** returns true if the account is supported (or unlocked) */
    public boolean hasAccount(String address) {
        return privateKeys.containsKey(address.toLowerCase());
    }

    /** signing of the raw data. */
    public String sign(String data, String address) {
        String key = privateKeys.get(address.toLowerCase());
        return signData(key, data);
    }

    private static native String getAddressFromKey(String key);

    private static native String signData(String key, String data);

    private static native String decodeKeystore(String keystore, String passwd);

}
