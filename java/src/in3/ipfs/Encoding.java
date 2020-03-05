package in3.ipfs;

public enum Encoding {
  IPFS_ENC_HEX(0),
  IPFS_ENC_UTF8(1),
  IPFS_ENC_B64(2);

  private final int econdingCode;

  Encoding(int econdingCode) {
    this.econdingCode = econdingCode;
  }

  public int getEncodeCode() {
    return econdingCode;
  }
}
