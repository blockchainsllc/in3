package in3.utils;

/*
 * Constants for the type of Signature
 */
public enum SignatureType {
  eth_sign(2),
  /**
   * < add Ethereum Signed Message-Proefix, hash and sign the data
   */
  raw(0),
  /**
   * < sign the data directly
   */
  hash(1);

  /**
   * < hash and sign the data
   */

  public final int val;

  private SignatureType(int val) {
    this.val = val;
  }

  private int getValue() {
    return this.val;
  }

  public static SignatureType getEnum(int enumVal) {
    for (SignatureType v : values())
      if (v.getValue() == enumVal) return v;
    return null;
  }
}
