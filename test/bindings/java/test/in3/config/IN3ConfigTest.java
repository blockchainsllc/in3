package in3.config;

import org.json.JSONObject;
import org.junit.jupiter.api.*;
import in3.*;

public class IN3ConfigTest {
    // This class is more akin to integrated tests since, as you can see, it relies on multiple Objects.
    private IN3 client;

    @BeforeEach
    public void createClient() {
        client = IN3.forChain(Chain.MAINNET);
    }

    @Test
    public void objectHierarchy() {
        final String nodeConfigOutputKey = "nodes";
        final String nodeListConfigOutputKey = "nodeList";

        ClientConfiguration clientConfig = new ClientConfiguration();

        NodeConfiguration nodeConfig = new NodeConfiguration(Chain.GOERLI, clientConfig);
        nodeConfig.setNeedsUpdate(false);
        nodeConfig.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
        nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
        nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");

        JSONObject jsonObject1 = new JSONObject(clientConfig.toJSON());
        Assertions.assertTrue(jsonObject1.has(nodeConfigOutputKey));

        NodeListConfiguration nodeListConfig = new NodeListConfiguration(nodeConfig);
        nodeListConfig.setProps("someProp");
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
    public void setConfigChangesNativeState() {
        // This is not the correct place for this test but it will suffice for this.
        ClientConfiguration config = new ClientConfiguration();

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

        client.setConfig(config);

        Assertions.assertTrue(requestCount == client.getRequestCount());
        Assertions.assertTrue(autoUpdateList == client.isAutoUpdateList());
        Assertions.assertTrue(proof == client.getProof());
        Assertions.assertTrue(maxAttempts == client.getMaxAttempts());
        Assertions.assertTrue(signatureCount == client.getSignatureCount());
    }

    @Test
    public void setConfigThrowsIllegalArgumentException() {
        // This is not the correct place for this test but it will suffice for this.
        ClientConfiguration config = new ClientConfiguration();

        NodeConfiguration nodeConfig = new NodeConfiguration(Chain.GOERLI, config);
        nodeConfig.setNeedsUpdate(false);
        nodeConfig.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");
        nodeConfig.setContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
        nodeConfig.setWhiteListContract("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab");
        nodeConfig.setWhiteList(new String[] {
           "0x0123456789012345678901234567890123456789",
           "0x1234567890123456789012345678901234567890"
        });

        Assertions.assertThrows(IllegalArgumentException.class, () -> {
           client.setConfig(config);
        });
    }
}
