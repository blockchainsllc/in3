package in3;

import in3.utils.JSON;

public class BlockID {
  private Long   number;
  private String hash;

  private BlockID() {}

  public static BlockID fromHash(String hash) {
    BlockID blockId = new BlockID();
    blockId.setHash(hash);

    return blockId;
  }

  public static BlockID fromNumber(long number) {
    BlockID blockId = new BlockID();
    blockId.setNumber(number);

    return blockId;
  }

  public Long getNumber() {
    return number;
  }

  public void setNumber(long block) {
    this.number = block;
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

    if (getNumber() != null) {
      JSON.appendKey(sb, "blockNumber", getNumber());
    }

    if (getHash() != null) {
      JSON.appendKey(sb, "hash", getHash());
    }

    sb.setCharAt(sb.length() - 1, '}');

    return sb.toString();
  }
}
