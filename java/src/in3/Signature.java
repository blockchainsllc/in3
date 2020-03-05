package in3;

import in3.utils.JSON;

public class Signature {

  private JSON data;

  private static final String MESSAGE      = "message";
  private static final String MESSAGE_HASH = "messageHash";
  private static final String SIGNATURE    = "signature";
  private static final String R            = "r";
  private static final String S            = "s";
  private static final String V            = "v";

  private Signature(JSON data) {
    this.data = data;
  }

  protected static Signature asSignature(Object o) {
    if (o == null)
      return null;
    return new Signature((JSON) o);
  }

  public String getMessage() {
    return data.getString(MESSAGE);
  }
  public String getMessageHash() {
    return data.getString(MESSAGE_HASH);
  }
  public String getSignature() {
    return data.getString(SIGNATURE);
  }
  public String getR() {
    return data.getString(R);
  }
  public String getS() {
    return data.getString(S);
  }
  public long getV() {
    return data.getLong(V);
  }
}
