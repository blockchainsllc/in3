package in3.eth1;

import in3.*;
import java.math.*;

/**
 * represents a Transaction in ethereum.
 * 
 */

public class Transaction {

    private JSON data;

    protected Transaction(JSON data) {
        this.data = data;
    }

    /**
     * the blockhash of the block containing this transaction.
     */
    public String getBlockHash() {
        return data.getString("blockHash");
    }

    /**
     * the block number of the block containing this transaction.
     */
    public long getBlockNumber() {
        return data.getLong("blockNumber");
    }

    /**
     * the chainId of this transaction.
     */
    public String getChainId() {
        return data.getString("chainId");
    }

    /**
     * the address of the deployed contract (if successfull)
     */
    public String getCreatedContractAddress() {
        return data.getString("creates");
    }

    /**
     * the address of the sender.
     */
    public String getFrom() {
        return data.getString("from");
    }

    /**
     * the Transaction hash.
     */
    public String getHash() {
        return data.getString("hash");
    }

    /**
     * the Transaction data or input data.
     */
    public String getData() {
        return data.getString("input");
    }

    /**
     * the nonce used in the transaction.
     */
    public long getNonce() {
        return data.getLong("nonce");
    }

    /**
     * the public key of the sender.
     */
    public String getPublicKey() {
        return data.getString("publicKey");
    }

    /**
     * the value send in wei.
     */
    public BigInteger getValue() {
        return data.getBigInteger("value");
    }

    /**
     * the raw transaction as rlp encoded data.
     */
    public String getRaw() {
        return data.getString("raw");
    }

    /**
     * the address of the receipient or contract.
     */
    public String getTo() {
        return data.getString("to");
    }

    /**
     * the signature of the sender - a array of the [ r, s, v]
     */
    public String[] getSignature() {
        return new String[] { data.getString("r"), data.getString("s"), data.getString("v") };
    }

    /**
     * the gas price provided by the sender.
     */
    public long getGasPrice() {
        return data.getLong("gasPrice");
    }

    /**
     * the gas provided by the sender.
     */
    public long getGas() {
        return data.getLong("gas");
    }
}
