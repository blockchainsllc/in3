package in3.eth1;

import in3.*;

/**
 * a log entry of a transaction receipt.
 * 
 */

public class Log {

    private JSON data;

    private Log(JSON data) {
        this.data = data;
    }

    protected static Log[] asLogs(Object o) {
        if (o == null)
            return null;
        if (o instanceof Object[]) {
            Object[] a = (Object[]) o;
            Log[] s = new Log[a.length];
            for (int i = 0; i < s.length; i++)
                s[i] = a[i] == null ? null : new Log((JSON) a[i]);
            return s;
        }
        return null;
    }

    protected static Log asLog(Object o) {
        if (o == null)
            return null;
        return new Log((JSON) o);
    }

    /**
     * true when the log was removed, due to a chain reorganization. false if its a
     * valid log.
     */
    public boolean isRemoved() {
        return (Boolean) data.get("removed");
    }

    /**
     * integer of the log index position in the block. null when its pending log.
     */
    public int getLogIndex() {
        return JSON.asInt(data.get("logIndex"));
    }

    /**
     * integer of the transactions index position log was created from. null when
     * its pending log.
     */
    public int gettTansactionIndex() {
        return JSON.asInt(data.get("transactionIndex"));
    }

    /**
     * Hash, 32 Bytes - hash of the transactions this log was created from. null
     * when its pending log.
     */
    public String getTransactionHash() {
        return data.getString("transactionHash");
    }

    /**
     * Hash, 32 Bytes - hash of the block where this log was in. null when its
     * pending. null when its pending log.
     */
    public String getBlockHash() {
        return data.getString("blockHash");
    }

    /**
     * the block number where this log was in. null when its pending. null when its
     * pending log.
     */
    public long getBlockNumber() {
        return data.getLong("blockNumber");
    }

    /**
     * 20 Bytes - address from which this log originated.
     */
    public String getAddress() {
        return data.getString("address");
    }

    /**
     * Array of 0 to 4 32 Bytes DATA of indexed log arguments. (In solidity: The
     * first topic is the hash of the signature of the event (e.g.
     * Deposit(address,bytes32,uint256)), except you declared the event with the
     * anonymous specifier.)
     */
    public String[] getTopics() {
        return data.getStringArray("topics");
    }

}
