package in3.eth1;

import in3.*;
import java.math.*;

/**
 * represents a Transaction Request which should be send or called.
 */

public class TransactionRequest {

    /** the from address */
    public String from;

    /** the recipients address */
    public String to;

    /** the data */
    public String data;

    /** the value of the transaction */
    public BigInteger value;

    /** the nonce (transactionCount of the sender) */
    public long nonce = -1;

    /** the gas to use */
    public long gas;

    /** the gas price to use */
    public long gasPrice;

    /** the signature for the function to call */
    public String function;

    /** the params to use for encoding in the data */
    public Object[] params;

    /**
     * creates the data based on the function/params values.
     */
    public String getData() {
        String result = data == null || data.length() < 2 ? "0x" : data;
        if (function != null) {
            String fnData = abiEncode(function, JSON.toJson(params));
            if (fnData != null && fnData.length() > 2 && fnData.startsWith("0x"))
                result += fnData.substring(2 + (result.length() > 2 ? 8 : 0));
        }
        return result;
    }

    public String getTransactionJson() {
        StringBuilder sb = new StringBuilder();
        sb.append("{");
        if (to != null)
            JSON.appendKey(sb, "to", to);
        if (gas > 0)
            JSON.appendKey(sb, "gasLimit", JSON.asString(gas));
        if (gasPrice > 0)
            JSON.appendKey(sb, "gasPrice", JSON.asString(gasPrice));
        if (value != null)
            JSON.appendKey(sb, "value", JSON.asString(value));
        if (nonce >= 0)
            JSON.appendKey(sb, "nonce", JSON.asString(nonce));
        JSON.appendKey(sb, "data", getData());
        sb.setCharAt(sb.length() - 1, '}');
        return sb.toString();
    }

    public Object getResult(String data) {
        if (function == null)
            return data;
        Object[] res = (Object[]) abiDecode(function, data);
        return res.length == 1 ? res[0] : res;
    }

    static {
        Loader.loadLibrary();
    }

    private static native String abiEncode(String function, String jsonParams);

    private static native Object abiDecode(String function, String data);

}