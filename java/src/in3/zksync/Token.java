package in3.zksync;

import in3.utils.JSON;

/**
 * A ZKSync Token.
 */
public class Token {

  private JSON data;

  private Token(JSON data) {
    this.data = data;
  }

  public static Token asToken(Object o) {
    if (o == null)
      return null;
    return new Token((JSON) o);
  }

  public static Token[] asTokens(Object o) {
    if (o == null)
      return null;
    Object[] a = (Object[]) o;
    Token[] b  = new Token[a.length];
    for (int i = 0; i < a.length; i++)
      b[i] = Token.asToken(a[i]);
    return b;
  }

  /**
   * The token-address if the String.
   */
  public String getAddress() {
    return data.getString("address");
  }

  /**
   * The decimals or number of digits after the comma.
   */
  public int getDecimals() {
    return data.getInteger("decimals");
  }

  /**
   * id
   */
  public int getId() {
    return data.getInteger("id");
  }

  /**
   * The Symbol.
   */
  public String getSize() {
    return data.getString("symbol");
  }
}
