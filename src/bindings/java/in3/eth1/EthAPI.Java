package in3.eth1;

import in3.*;
import in3.eth1.*;

import java.math.BigInteger;

/**
 * a Wrapper for the incubed client offering Type-safe Access and additional
 * helper functions.
 */
public class EthAPI {

    private IN3 in3;

    /**
     * creates a API using the given incubed instance.
     */
    public EthAPI(IN3 in3) {
        this.in3 = in3;
    }

    /**
     * finds the Block as specified by the number. use `Block.LATEST` for getting
     * the lastest block.
     */
    public Block getBlockByNumber(long block, /** < the Blocknumber */
    boolean includeTransactions /**
                                 * < if true all Transactions will be includes, if not only the
                                 * transactionhashes
                                 */
    ) {
        return new Block((JSON) in3.sendRPCasObject("eth_getBlockByNumber",
                new Object[] { getBlockString(block), includeTransactions }));
    }

    /**
     * the current BlockNumber.
     */
    public long getBlockNumber() {
        return JSON.asLong(in3.sendRPCasObject("eth_blockNumber", new Object[] {}));
    }

    /**
     * the current Gas Price.
     */
    public long getGasPrice() {
        return JSON.asLong(in3.sendRPCasObject("eth_gasPrice", new Object[] {}));
    }

    /**
     * calls a function of a smart contract and returns the result.
     * 
     * @return the decoded result. if only one return value is expected the Object
     *         will be returned, if not an array of objects will be the result.
     */
    public Object call(TransactionRequest request, /** < the transaction to call. */
    long block /** < the Block used to for the state. */
    ) {
        return request.getResult((String) in3.sendRPCasObject("eth_call",
                new Object[] { request.getTransactionJson(), getBlockString(block) }));
    }

    /**
     * the current Gas Price.
     * 
     * @return the decoded result. if only one return value is expected the Object
     *         will be returned, if not an array of objects will be the result.
     */
    public Object call(String to, String function, Object... params) {
        TransactionRequest req = new TransactionRequest();
        req.to = to;
        req.function = function;
        req.params = params;
        return call(req, Block.LATEST);
    }

    private static String getBlockString(long l) {
        return l == Block.LATEST ? "latest" : "0x" + Long.toHexString(l);
    }

}
