package in3;

import in3.config.ChainConfiguration;
import in3.config.ClientConfiguration;
import in3.config.NodeConfiguration;

public class IN3MockBuilder {
  private IN3  client;
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
    for (String[] fileNameTuple : fileNameTuples) {
      newtransport.setMockResponse(fileNameTuple[0], fileNameTuple[1]);
    }

    client.setTransport(newtransport);
  }

  public void buildConfig() {
    ClientConfiguration clientConfig = client.getConfig();

    ChainConfiguration chainConfig1 = new ChainConfiguration(Chain.MAINNET, clientConfig);
    chainConfig1.setNeedsUpdate(false);
    chainConfig1.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    chainConfig1.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

    ChainConfiguration chainConfig2 = new ChainConfiguration(Chain.GOERLI, clientConfig);
    chainConfig2.setNeedsUpdate(false);
    chainConfig2.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    chainConfig2.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

    // Necessary for nodelist test to pass
    NodeConfiguration nodeConfig = new NodeConfiguration(chainConfig2);
    nodeConfig.setUrl("https://in3-v2.slock.it/goerli/nd-2");
    nodeConfig.setAddress("0x1fe2e9bf29aa1938859af64c413361227d04059a");
    nodeConfig.setProps(0);

    ChainConfiguration chainConfig3 = new ChainConfiguration(Chain.KOVAN, clientConfig);
    chainConfig3.setNeedsUpdate(false);
    chainConfig3.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    chainConfig3.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

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
