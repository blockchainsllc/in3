package in3.btc;

import in3.utils.JSON;

/**
 * Script on a transaction output.
 */
public class ScriptPubKey {
  private JSON data;

  private ScriptPubKey(JSON data) {
    this.data = data;
  }

  protected static ScriptPubKey asScriptPubKey(Object o) {
    if (o == null)
      return null;
    return new ScriptPubKey((JSON) o);
  }

  protected static ScriptPubKey[] asScriptPubKeys(Object o) {
    if (o == null)
      return null;
    Object[] a       = (Object[]) o;
    ScriptPubKey[] b = new ScriptPubKey[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = ScriptPubKey.asScriptPubKey(a[i]);
    return b;
  }

  /**
   * The hash of the blockheader.
   */
  public String getAsm() {
    return data.getString("asm");
  }

  /**
   * The raw hex data.
   */
  public String getHex() {
    return data.getString("hex");
  }

  /**
   * The required sigs.
   */
  public long getReqSigs() {
    return data.getLong("reqSigs");
  }

  /**
   * The type e.g.: pubkeyhash.
   */
  public String getType() {
    return data.getString("type");
  }

  /**
   * List of addresses.
   */
  public String[] getAddresses() {
    return data.getStringArray("addresses");
  }
}
