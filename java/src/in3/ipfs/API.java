package in3.ipfs;

import in3.IN3;
import in3.utils.JSON;
import java.nio.charset.Charset;

/**
   * API for ipfs custom methods. To be used along with "Chain.IPFS" on in3 instance.
   */
public class API {
  private IN3 in3;

  private static final String PUT = "ipfs_put";
  private static final String GET = "ipfs_get";

  private enum Enconding {
    base64,
    hex,
    utf8;
  }

  /**
     * creates a ipfs.API using the given incubed instance.
     */
  public API(IN3 in3) {
    this.in3 = in3;
  }

  /**
   * Returns the content associated with specified multihash on success OR NULL on error.
   */
  public byte[] get(String multihash) {
    if (multihash == null) throw new IllegalArgumentException();

    String content = JSON.asString(in3.sendRPCasObject(GET, new Object[] {
                                                                multihash,
                                                                JSON.asString(Enconding.base64)}));

    return content != null ? base64Decode(content) : null;
  }

  /**
   * Returns the IPFS multihash of stored content on success OR NULL on error.
   */
  public String put(String content) {
    return put(content != null ? content.getBytes(Charset.forName("UTF8")) : null);
  }

  /**
   * Returns the IPFS multihash of stored content on success OR NULL on error.
   */
  public String put(byte[] content) {
    if (content == null) throw new IllegalArgumentException();

    String encodedContent = base64Encode(content);
    Object rawResult      = in3.sendRPCasObject(PUT, new Object[] {
                                                    encodedContent});

    return JSON.asString(rawResult);
  }

  private native        byte[] base64Decode(String str);
  private native String base64Encode(byte[] str);
}
