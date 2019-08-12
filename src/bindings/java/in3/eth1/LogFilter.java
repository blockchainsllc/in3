package in3.eth1;

import in3.JSON;

/**
 * Log configuration for search logs.
 */

public class LogFilter {

    /**
     * Quantity or Tag - (optional) (default: latest) Integer block number, or
     * 'latest' for the last mined block or 'pending', 'earliest' for not yet mined
     * transactions.
     */
    long fromBlock = Block.LATEST;
    /**
     * Quantity or Tag - (optional) (default: latest) Integer block number, or
     * 'latest' for the last mined block or 'pending', 'earliest' for not yet mined
     * transactions.
     */
    long toBlock = Block.LATEST;
    /**
     * (optional) 20 Bytes - Contract address or a list of addresses from which logs
     * should originate.
     */
    String address;
    /**
     * (optional) Array of 32 Bytes Data topics. Topics are order-dependent. It’s
     * possible to pass in null to match any topic, or a subarray of multiple topics
     * of which one should be matching.
     */
    Object[] topics;

    /** a(optional) The maximum number of entries to retrieve (latest first). */
    int limit;

    /**
     * creates a JSON-String.
     */
    public String toString() {
        StringBuilder sb = new StringBuilder('{');
        if (fromBlock >= 0)
            JSON.appendKey(sb, "fromBlock", fromBlock);
        if (toBlock >= 0)
            JSON.appendKey(sb, "toBlock", toBlock);
        if (topics != null)
            JSON.appendKey(sb, "topics", JSON.toJson(topics));
        if (limit > 0)
            JSON.appendKey(sb, "limit", JSON.asString(limit));
        if (address != null)
            JSON.appendKey(sb, "address", JSON.asString(address));
        sb.setCharAt(sb.length() - 1, '}');
        return sb.toString();
    }

}