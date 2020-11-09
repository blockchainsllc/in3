package in3;

import in3.config.ClientConfiguration;
import in3.config.NodeRegistryConfiguration;

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
