package in3.eth1;
import in3.*;
import in3.eth1.*;

public class EthAPI {

    private IN3 in3;

    EthAPI(IN3 in3) {
        this.in3 = in3;
    }
    public Block getBlockByNumber(long block, boolean includeTransactions) {
        return new Block((JSON)in3.sendRPCasObject("eth_getBlockByNumber",new Object[]{ getBlockString(block),includeTransactions }));
    }

    private String getBlockString(long l) {
        return l==Block.LATEST ? "latest" :  "0x"+Long.toHexString(l);
    }

}
