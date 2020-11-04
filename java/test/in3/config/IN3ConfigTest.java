package in3.config;

import in3.*;
import in3.utils.JSON;
import org.json.JSONObject;
import org.junit.jupiter.api.*;

public class IN3ConfigTest {
  // This class is more akin to integrated tests since, as you can see, it relies
  // on multiple Objects.
  private IN3 client;

  @BeforeEach
  public void createClient() {
    client = IN3.forChain(Chain.MAINNET);
  }

  @Test
  public void objectHierarchy() {
    final String nodeConfigOutputKey = "nodeRegistry";
    final String whiteListConfigOutputKey = "whiteListContract";

    ClientConfiguration clientConfig = client.getConfig();

    NodeRegistryConfiguration nodeConfig = clientConfig.getNodeRegistry();
    nodeConfig.setNeedsUpdate(false);
    nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
    nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");

    String json = clientConfig.toJSON();
    JSONObject jsonObject1 = new JSONObject(json);
    Assertions.assertTrue(jsonObject1.has(nodeConfigOutputKey));
    Assertions.assertTrue(jsonObject1.getJSONObject(nodeConfigOutputKey).has(whiteListConfigOutputKey));
  }

  @Test
  public void isSynced() {
    ClientConfiguration config = client.getConfig();
    Assertions.assertTrue(config.isSynced());

    int requestCount = 1;
    boolean autoUpdateList = false;
    Proof proof = Proof.none;
    int maxAttempts = 1;
    int signatureCount = 0;

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
  public void addNodeConfiguration() {
    final String nodesOutputKey = "nodeList";
    ClientConfiguration config = client.getConfig();
    NodeRegistryConfiguration nodeRegistryConfig = config.getNodeRegistry();
    NodeConfiguration nodeConf = new NodeConfiguration(nodeRegistryConfig);
    nodeConf.setUrl("http://localnode.com");
    nodeConf.setAddress("0x798b7c7da709d7");
    nodeConf.setProps(0);
    Assertions.assertTrue(nodeRegistryConfig.getNodesConfiguration().length > 0);

    Assertions.assertTrue(config.toJSON().contains(nodesOutputKey));
  }

  @Test
  public void dispatchRequestInvalidConfig() {
    ClientConfiguration config = client.getConfig();

    NodeRegistryConfiguration nodeConfig = config.getNodeRegistry();
    nodeConfig.setNeedsUpdate(false);
    nodeConfig.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
    nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
    nodeConfig.setWhiteList(
        new String[] { "0x0123456789012345678901234567890123456789", "0x1234567890123456789012345678901234567890" });

    Assertions.assertThrows(IllegalArgumentException.class, () -> {
      client.getEth1API().getGasPrice();
    });
  }
}
