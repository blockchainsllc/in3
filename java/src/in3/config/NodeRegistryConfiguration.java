package in3.config;

import java.util.ArrayList;

import in3.utils.JSON;

/**
 * Part of the configuration hierarchy for IN3 Client. Holds the configuration a
 * node group in a particular Chain.
 */
public class NodeRegistryConfiguration implements Configuration {
  private JSON data;
  private boolean dirty;
  private ArrayList<Configuration> nodesConfiguration;

  protected NodeRegistryConfiguration(Object json) {
    this.data = (JSON) json;
    this.nodesConfiguration = new ArrayList<Configuration>();
    markAsSynced();
  }

  public Boolean isNeedsUpdate() {
    return this.data.getBoolean("needsUpdate");
  }

  /* preemptively update the nodelist */
  public void setNeedsUpdate(boolean needsUpdate) {
    setDirty(needsUpdate != isNeedsUpdate());
    this.data.put("needsUpdate", needsUpdate);
  }

  public String getContract() {
    return this.data.getString("contract");
  }

  /* the address of the registry contract */
  public void setContract(String contract) {
    setDirty(!isObjectEqual(contract, getContract()));
    this.data.put("contract", contract);
  }

  public String getRegistryId() {
    return this.data.getString("registryId");
  }

  /* the identifier of the registry */
  public void setRegistryId(String registryId) {
    setDirty(!isObjectEqual(registryId, getRegistryId()));
    this.data.put("registryId", registryId);
  }

  public String getWhiteListContract() {
    return this.data.getString("whiteListContract");
  }

  /*
   * address of whiteList contract. If specified, whiteList is always auto-updated
   * and manual whiteList is overridden
   */
  public void setWhiteListContract(String whiteListContract) {
    setDirty(!isObjectEqual(whiteListContract, getWhiteListContract()));
    this.data.put("whiteListContract", whiteListContract);
  }

  public String[] getWhiteList() {
    return this.data.getStringArray("whiteList");
  }

  /* array of node addresses that constitute the whiteList */
  public void setWhiteList(String[] whiteList) {
    setDirty(isObjectEqual(whiteList, getWhiteList()));
    this.data.put("whiteList", whiteList);
  }

  public String toJSON() {
    return JSON.toJson(this);
  }

  public boolean isSynced() {
    return !isDirty();
  }

  public void markAsSynced() {
    dirty = false;
  }

  /*
   * returns a COPY of the nodes array with the applied configuration
   * 
   * assert nodeConfiguration.getNodesConfiguration() != nodeConfiguration.getNodesConfiguration();
   * assert Arrays.equals(nodeConfiguration.getNodesConfiguration(), nodeConfiguration.getNodesConfiguration());
   */
  public NodeConfiguration[] getNodesConfiguration() {
    NodeConfiguration[] array = new NodeConfiguration[nodesConfiguration.size()];
    return nodesConfiguration.toArray(array);
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("{");

    if (isNeedsUpdate() != null) {
      JSON.appendKey(sb, "needsUpdate", isNeedsUpdate());
    }
    if (getContract() != null) {
      JSON.appendKey(sb, "contract", getContract());
    }

    if (getContract() != null) {
      JSON.appendKey(sb, "registryId", getRegistryId());
    }

    if (getWhiteList() != null && getWhiteList().length > 0) {
      JSON.appendKey(sb, "whiteList", getWhiteList());
    }

    if (getWhiteListContract() != null) {
      JSON.appendKey(sb, "whiteListContract", getWhiteListContract());
    }
    
    if (getNodesConfiguration().length > 0) {
      JSON.appendKey(sb, "nodeList", getNodesConfiguration());
    }

    sb.setCharAt(sb.length() - 1, '}');

    return sb.toString();
  }
  
  protected void addNodeConfiguration(Configuration conf) {
    this.nodesConfiguration.add(conf);
  }

  private void setDirty(boolean dirty) {
    this.dirty = isDirty() || dirty;
  }

  private boolean isObjectEqual(Object obj1, Object obj2) {
    return obj1 == obj2 || (obj1 != null && obj1.equals(obj2));
  }

  private boolean isDirty() {
    return dirty;
  }
}
