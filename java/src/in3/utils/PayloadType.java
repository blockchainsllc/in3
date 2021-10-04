package in3.utils;

/*
 * Enum that represents a type of payload for the signer
 */
public enum PayloadType {
  PL_SIGN_ANY(0),    /**< custom data to be signed*/
  PL_SIGN_ETHTX(1),  /**< the payload is a ethereum-tx */
  PL_SIGN_BTCTX(2),  /**< the payload is a BTC-Tx-Input */
  PL_SIGN_SAFETX(3); /**< The payload is a rlp-encoded data of a Gnosys Safe Tx */

  public final int val;

  private PayloadType(int val) {
    this.val = val;
  }

  private int getValue() {
    return this.val;
  }

  public static PayloadType getEnum(int enumVal) {
    for (PayloadType v : values())
      if (v.getValue() == enumVal) return v;
    return null;
  }
}