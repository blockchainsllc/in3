package in3;

import in3.utils.JSON;

public class VerifiedHash {
    private long block;
    private String hash;

    public VerifiedHash(long block, String hash) {
        this.block = block;
        this.hash = hash;
    }

    public long getBlock() {
        return block;
    }

    public void setBlock(long block) {
        this.block = block;
    }

    public String getHash() {
        return hash;
    }

    public void setHash(String hash) {
        this.hash = hash;
    }

    public String toJSON() {
        return JSON.toJson(this);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("{");

        JSON.appendKey(sb, "blockNumber", getBlock());
        JSON.appendKey(sb, "hash", getHash());
        sb.setCharAt(sb.length() - 1, '}');

        return sb.toString();
    }
}
