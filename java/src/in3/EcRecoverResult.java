package in3;

import in3.utils.JSON;

public class EcRecoverResult {
  private JSON data;

  private EcRecoverResult(JSON data) {
    this.data = data;
  }

  protected static EcRecoverResult asEcRecoverResult(Object o) {
    if (o == null)
      return null;
    return new EcRecoverResult((JSON) o);
  }

  public String getAddress() {
    return data.getString("address");
  }

  public String getPublicKey() {
    return data.getString("publicKey");
  }
}
