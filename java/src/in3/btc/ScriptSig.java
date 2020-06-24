package in3.btc;

import in3.utils.JSON;

/**
 * Script on a transaction input.
 */
public class ScriptSig {
  private JSON data;

  public ScriptSig(JSON data) {
    this.data = data;
  }

  protected static ScriptSig asScriptSig(Object o) {
    if (o == null)
      return null;
    return new ScriptSig((JSON) o);
  }

  protected static ScriptSig[] asScriptSigs(Object o) {
    if (o == null)
      return null;
    Object[] a    = (Object[]) o;
    ScriptSig[] b = new ScriptSig[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = ScriptSig.asScriptSig(a[i]);
    return b;
  }

  /**
   * The asm data.
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
}
