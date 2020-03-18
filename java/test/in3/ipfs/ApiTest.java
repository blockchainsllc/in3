package in3.ipfs;

import in3.Chain;
import in3.IN3;
import in3.IN3MockBuilder;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class ApiTest {

  private IN3 in3;

  @BeforeEach
  public void setupClient() {
    String[][] mockedResponses = {
        {"ipfs_get", "ipfs_get.json"},
        {"ipfs_put", "ipfs_put.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.IPFS);
    in3                    = builder.constructClient(mockedResponses);
  }

  @Test
  public void put_success() {
    String content   = "Lorem ipsum dolor sit amet";
    String multihash = in3.getIpfs().put(content);
    Assertions.assertNotNull(multihash);
    Assertions.assertEquals("QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8", multihash);
  }

  @Test
  public void put_failure() {
    String nullable = null;
    Assertions.assertThrows(IllegalArgumentException.class, () -> {
      in3.getIpfs().put(nullable);
    });
  }

  @Test
  public void get_success() {
    String multihash = "QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8";
    byte[] content   = in3.getIpfs().get(multihash);
    Assertions.assertNotNull(content);
    Assertions.assertEquals("Lorem ipsum dolor sit amet", new String(content));
  }

  @Test
  public void get_failure() {
    String multihash = null;
    Assertions.assertThrows(IllegalArgumentException.class, () -> {
      in3.getIpfs().get(multihash);
    });
  }
}
