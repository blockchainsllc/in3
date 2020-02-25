package in3;

import in3.config.ClientConfiguration;
import in3.config.NodeConfiguration;

public class IN3MockBuilder {
    private IN3 client;
    private long chainId;

    public IN3MockBuilder(long chainId) {
        this.chainId = chainId;
    }

    public void createNewClient() {
           client = IN3.forChain(getChainId());
    }

    public void buildTransport(String[][] fileNameTuples) {
        IN3MockTransport newtransport = new IN3MockTransport();

        // TODO Consider on using Apache Commons Pair
        for (String[] fileNameTuple: fileNameTuples) {
            newtransport.setMockResponse(fileNameTuple[0], fileNameTuple[1]);
        }

        client.setTransport(newtransport);
    }

    public void buildConfig() {
        ClientConfiguration clientConfig = client.getConfig();

        NodeConfiguration nodeConfig1 = new NodeConfiguration(Chain.MAINNET, clientConfig);
        nodeConfig1.setNeedsUpdate(false);
        nodeConfig1.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig1.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

        NodeConfiguration nodeConfig2 = new NodeConfiguration(Chain.GOERLI, clientConfig);
        nodeConfig2.setNeedsUpdate(false);
        nodeConfig2.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig2.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

        NodeConfiguration nodeConfig3 = new NodeConfiguration(Chain.KOVAN, clientConfig);
        nodeConfig3.setNeedsUpdate(false);
        nodeConfig3.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig3.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

        clientConfig.setRequestCount(1);
        clientConfig.setAutoUpdateList(false);
        clientConfig.setProof(Proof.none);
        clientConfig.setMaxAttempts(1);
        clientConfig.setSignatureCount(0);
    }

    public IN3 constructClient(String[][] fileNameTuples) {
        createNewClient();
        buildTransport(fileNameTuples);
        buildConfig();
        return client;
    }

    private long getChainId() {
        return chainId;
    }
}
