package in3.config;

import java.util.ArrayList;

import in3.utils.JSON;

/**
 * Part of the configuration hierarchy for IN3 Client. Holds the configuration
 * a node group in a particular Chain.
 */
public class NodeConfiguration implements Configuration {

    private long chain;
    private Boolean needsUpdate;
    private String contract;
    private String registryId;
    private String whiteListContract;
    private String[] whiteList = new String[] {};
    public ArrayList<NodeListConfiguration> nodeList = new ArrayList<NodeListConfiguration>();

    public NodeConfiguration(long chain, ClientConfiguration config) {
        this.chain = chain;
        config.addNodeConfiguration(this);
    }

    public long getChain() {
        return chain;
    }

    public Boolean isNeedsUpdate() {
        return needsUpdate;
    }

    /* preemptively update the nodelist */
    public void setNeedsUpdate(boolean needsUpdate) {
        this.needsUpdate = needsUpdate;
    }

    public String getContract() {
        return contract;
    }

    /* the address of the registry contract */
    public void setContract(String contract) {
        this.contract = contract;
    }

    public String getRegistryId() {
        return registryId;
    }

    /* the identifier of the registry */
    public void setRegistryId(String registryId) {
        this.registryId = registryId;
    }

    public String getWhiteListContract() {
        return whiteListContract;
    }

    /* address of whiteList contract. If specified, whiteList is always auto-updated and manual whiteList is overridden */
    public void setWhiteListContract(String whiteListContract) {
        this.whiteListContract = whiteListContract;
    }

    public String[] getWhiteList() {
        return whiteList;
    }

    /* array of node addresses that constitute the whiteList */
    public void setWhiteList(String[] whiteList) {
        this.whiteList = whiteList;
    }

    protected void addNodeListConfiguration(NodeListConfiguration configuration) {
        nodeList.add(configuration);
    }

    private ArrayList<NodeListConfiguration> getNodeList() {
        return nodeList;
    }

    public String toJSON() {
        return JSON.toJson(this);
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

        if (getNodeList().size() > 0) {
            JSON.appendKey(sb, "nodeList", getNodeList());
        }

        if (getWhiteList().length > 0) {
            JSON.appendKey(sb, "whiteList", getWhiteList());
        }

        if (getWhiteListContract() != null) {
            JSON.appendKey(sb, "whiteListContract", getWhiteListContract());
        }

        sb.setCharAt(sb.length() - 1, '}');

        return sb.toString();
    }
}
