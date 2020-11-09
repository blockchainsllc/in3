package in3.config;

import in3.Proof;
import in3.utils.JSON;
import java.math.BigInteger;

/**
 * Configuration Object for Incubed Client. It holds the state for the root of
 * the configuration tree. Should be retrieved from the client instance as
 * IN3#getConfig()
 */
public class ClientConfiguration implements Configuration {
  private JSON                      data;
  private NodeRegistryConfiguration registryConfiguration;

  private boolean dirty;

  // Make the constructor private in order to ensure people use client.getConfig()
  public ClientConfiguration(Object json) {
    this.data = (JSON) json;
    setNodeRegistry();
    markAsSynced();
  }

  public Integer getRequestCount() {
    return data.getInteger("requestCount");
  }

  /** sets the number of requests send when getting a first answer */
  public void setRequestCount(Integer requestCount) {
    setDirty(requestCount != getRequestCount());
    data.put("requestCount", requestCount);
  }

  public Boolean isAutoUpdateList() {
    return data.getBoolean("autoUpdateList");
  }

  /**
   * activates the auto update.if true the nodelist will be automatically updated
   * if the lastBlock is newer
   */
  public void setAutoUpdateList(boolean autoUpdateList) {
    setDirty(autoUpdateList != isAutoUpdateList());
    data.put("autoUpdateList", autoUpdateList);
  }

  public Proof getProof() {
    String proof = data.getString("proof");
    return proof != null ? Proof.valueOf(proof) : null;
  }

  /** sets the type of proof used */
  public void setProof(Proof proof) {
    setDirty(!isObjectEqual(getProof(), proof));
    data.put("proof", proof.toString());
  }

  public Integer getMaxAttempts() {
    return data.getInteger("maxAttempts");
  }

  /** sets the max number of attempts before giving up */
  public void setMaxAttempts(int maxAttempts) {
    setDirty(maxAttempts != getMaxAttempts());
    data.put("maxAttempts", maxAttempts);
  }

  public Integer getSignatureCount() {
    return data.getInteger("signatureCount");
  }

  /** sets the number of signatures used to proof the blockhash. */
  public void setSignatureCount(int signatureCount) {
    setDirty(signatureCount != getSignatureCount());
    data.put("signatureCount", signatureCount);
  }

  public Integer getFinality() {
    return data.getInteger("finality");
  }

  /** sets the number of signatures in percent required for the request */
  public void setFinality(int finality) {
    setDirty(finality != getFinality());
    data.put("finality", finality);
  }

  public Boolean isIncludeCode() {
    return data.getBoolean("includeCode");
  }

  public void setIncludeCode(boolean includeCode) {
    setDirty(includeCode != isIncludeCode());
    data.put("includeCode", includeCode);
  }

  public Boolean isBootWeights() {
    return data.getBoolean("bootWeights");
  }

  /**
   * if true, the first request (updating the nodelist) will also fetch the
   * current health status and use it for blacklisting unhealthy nodes. This is
   * used only if no nodelist is available from cache.
   */
  public void setBootWeights(boolean bootWeights) {
    setDirty(bootWeights != isBootWeights());
    data.put("bootWeights", bootWeights);
  }

  public Boolean isKeepIn3() {
    return data.getBoolean("keepIn3");
  }

  /* preserve in3 section of the rpc call response intact */
  public void setKeepIn3(boolean keepIn3) {
    setDirty(keepIn3 != isKeepIn3());
    data.put("keepIn3", keepIn3);
  }

  public Boolean isUseHttp() {
    return data.getBoolean("useHttp");
  }

  /* allow transport to use non-ssl */
  public void setUseHttp(boolean useHttp) {
    setDirty(useHttp != isUseHttp());
    data.put("useHttp", useHttp);
  }

  public Long getTimeout() {
    return data.getLong("timeout");
  }

  /**
   * specifies the number of milliseconds before the request times out. increasing
   * may be helpful if the device uses a slow connection.
   */
  public void setTimeout(long timeout) {
    setDirty(timeout != getTimeout());
    data.put("timeout", timeout);
  }

  public Long getMinDeposit() {
    return data.getLong("minDeposit");
  }

  /**
   * sets min stake of the server. Only nodes owning at least this amount will be
   * chosen.
   */
  public void setMinDeposit(long minDeposit) {
    setDirty(minDeposit != getMinDeposit());
    data.put("minDeposit", minDeposit);
  }

  public Long getNodeProps() {
    return data.getLong("nodeProps");
  }

  /* used to filter nodes according to its capabilities */
  public void setNodeProps(long nodeProps) {
    setDirty(nodeProps != getNodeProps());
    data.put("nodeProps", nodeProps);
  }

  public Long getNodeLimit() {
    return data.getLong("nodeLimit");
  }

  /** sets the limit of nodes to store in the client. */
  public void setNodeLimit(long nodeLimit) {
    setDirty(nodeLimit != getNodeLimit());
    data.put("nodeLimit", nodeLimit);
  }

  public NodeRegistryConfiguration getNodeRegistry() {
    return this.registryConfiguration;
  }

  public Integer getReplaceLatestBlock() {
    return data.getInteger("replaceLatestBlock");
  }

  /** replaces the *latest* with blockNumber- specified value */
  public void setReplaceLatestBlock(int replaceLatestBlock) {
    setDirty(replaceLatestBlock != getReplaceLatestBlock());
    data.put("replaceLatestBlock", replaceLatestBlock);
  }

  public String getRpc() {
    return data.getString("rpc");
  }

  /**
   * setup an custom rpc source for requests by setting Chain to local and proof
   * to none
   */
  public void setRpc(String rpc) {
    setDirty(isObjectEqual(rpc, getRpc()));
    data.put("rpc", rpc);
  }

  public boolean isSynced() {
    return !isDirty() && registryConfiguration.isSynced();
  }

  public void markAsSynced() {
    dirty = false;
    registryConfiguration.markAsSynced();
  }

  public String toString() {
    StringBuilder sb = new StringBuilder("{");

    if (getRequestCount() != null) {
      JSON.appendKey(sb, "requestCount", getRequestCount());
    }
    if (isAutoUpdateList() != null) {
      JSON.appendKey(sb, "autoUpdateList", isAutoUpdateList());
    }
    if (getProof() != null) {
      JSON.appendKey(sb, "proof", JSON.asString(getProof()));
    }
    if (getMaxAttempts() != null) {
      JSON.appendKey(sb, "maxAttempts", getMaxAttempts());
    }
    if (getSignatureCount() != null) {
      JSON.appendKey(sb, "signatureCount", getSignatureCount());
    }
    if (getFinality() != null) {
      JSON.appendKey(sb, "finality", getFinality());
    }
    if (isIncludeCode() != null) {
      JSON.appendKey(sb, "includeCode", isIncludeCode());
    }
    if (isBootWeights() != null) {
      JSON.appendKey(sb, "bootWeights", isBootWeights());
    }
    if (isKeepIn3() != null) {
      JSON.appendKey(sb, "keepIn3", isKeepIn3());
    }
    if (isUseHttp() != null) {
      JSON.appendKey(sb, "useHttp", isUseHttp());
    }
    if (getTimeout() != null) {
      JSON.appendKey(sb, "timeout", getTimeout());
    }
    if (getMinDeposit() != null) {
      JSON.appendKey(sb, "minDeposit", getMinDeposit());
    }
    if (getNodeProps() != null) {
      JSON.appendKey(sb, "nodeProps", getNodeProps());
    }
    if (getNodeLimit() != null) {
      JSON.appendKey(sb, "nodeLimit", getNodeLimit());
    }
    if (getReplaceLatestBlock() != null) {
      JSON.appendKey(sb, "replaceLatestBlock", getReplaceLatestBlock());
    }
    if (getRpc() != null) {
      JSON.appendKey(sb, "rpc", getRpc());
    }

    JSON.appendKey(sb, "nodeRegistry", getNodeRegistry());
    sb.setCharAt(sb.length() - 1, '}');
    return sb.toString();
  }

  @Override
  public String toJSON() {
    return this.toString();
  }

  private boolean isDirty() {
    return dirty;
  }

  private void setDirty(boolean dirty) {
    this.dirty = isDirty() || dirty;
  }

  private boolean isObjectEqual(Object obj1, Object obj2) {
    return obj1 == obj2 || (obj1 != null && obj1.equals(obj2));
  }

  private void setNodeRegistry() {
    JSON data                  = (JSON) this.data.getObject("nodeRegistry");
    this.registryConfiguration = new NodeRegistryConfiguration(data);
  }
}
