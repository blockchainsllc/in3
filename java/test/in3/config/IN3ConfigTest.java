package in3.config;

import in3.*;
import in3.utils.JSON;
import org.json.JSONObject;
import org.junit.jupiter.api.*;

public class IN3ConfigTest {
  // This class is more akin to integrated tests since, as you can see, it relies on multiple Objects.
  private IN3 client;

  @BeforeEach
  public void createClient() {
    client = IN3.forChain(Chain.MAINNET);
  }

  @Test
  public void objectHierarchy() {
    final String nodeConfigOutputKey     = "nodes";
    final String nodeListConfigOutputKey = "nodeList";

    ClientConfiguration clientConfig = client.getConfig();

    ChainConfiguration nodeConfig = new ChainConfiguration(Chain.GOERLI, clientConfig);
    nodeConfig.setNeedsUpdate(false);
    nodeConfig.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
    nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");

    JSONObject jsonObject1 = new JSONObject(clientConfig.toJSON());
    Assertions.assertTrue(jsonObject1.has(nodeConfigOutputKey));

    NodeConfiguration nodeListConfig = new NodeConfiguration(nodeConfig);
    nodeListConfig.setProps(Long.valueOf(0x0));
    nodeListConfig.setUrl("scheme://userinfo@host:port/path?query#fragment");
    nodeListConfig.setAddress("0x0");

    JSONObject jsonObject2 = new JSONObject(clientConfig.toJSON());
    Assertions.assertTrue(
        jsonObject2
            .getJSONObject(nodeConfigOutputKey)
            .getJSONObject(JSON.asString(Chain.GOERLI))
            .has(nodeListConfigOutputKey));
  }

  @Test
  public void isSynced() {
    ClientConfiguration config = client.getConfig();
    Assertions.assertTrue(config.isSynced());

    int     requestCount   = 1;
    boolean autoUpdateList = false;
    Proof   proof          = Proof.none;
    int     maxAttempts    = 1;
    int     signatureCount = 0;

    config.setRequestCount(requestCount);
    config.setAutoUpdateList(autoUpdateList);
    config.setProof(proof);
    config.setMaxAttempts(maxAttempts);
    config.setSignatureCount(signatureCount);

    Assertions.assertTrue(!config.isSynced());
    client.getEth1API().getChainId();
    Assertions.assertTrue(config.isSynced());
  }

  @Test
  public void dispatchRequestInvalidConfig() {
    ClientConfiguration config = client.getConfig();

    ChainConfiguration nodeConfig = new ChainConfiguration(Chain.GOERLI, config);
    nodeConfig.setNeedsUpdate(false);
    nodeConfig.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
    nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteList(new String[] {
        "0x0123456789012345678901234567890123456789",
        "0x1234567890123456789012345678901234567890"});

    Assertions.assertThrows(IllegalArgumentException.class, () -> {
      client.getEth1API().getGasPrice();
    });
  }
}
