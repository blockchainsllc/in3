package in3.eth1;

import in3.*;

public class Block {

    public static long LATEST = -1;
    public static long EARLIEST = 0;

    private JSON data;

    Block(JSON data) {
        this.data=data;
    }

    public String getGasLimit() {
        return (String)data.get("gasLimit");
    }
    
    public String getExtraData() {
        return (String)data.get("extraData");
    }
    
    public long getDifficulty() {
        return data.getLong("difficulty");
    }
    
    public String getAuthor() {
        return (String)data.get("author");
    }
    
    public String getTransactionsRoot() {
        return (String)data.get("transactionsRoot");
    }

    public String[] getUncles() {
        return data.getStringArray("uncles");
    }

       // Test it
   public static void main(String[] args) {
       EthAPI api = new EthAPI(new IN3());
       Block b = api.getBlockByNumber(Block.LATEST,false);
       System.out.println("miner="+b.getAuthor());
       System.out.println("difficulty="+b.getDifficulty());
   }
}
