package in3.utils;

/*
 * Constants for the type of Signature
 */
public enum CurveType {
  /**< sign with ecdsa */
  ecdsa(1),
  /**< use ed25519 curve */
  ed25519(2);

  public final int val;

  private CurveType(int val) {
    this.val = val;
  }

  private int getValue() {
    return this.val;
  }

  public static CurveType getEnum(int enumVal) {
    for (CurveType v : values())
      if (v.getValue() == enumVal) return v;
    return null;
  }
}
