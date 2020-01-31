package in3.config;

import java.util.ArrayList;
import java.util.HashMap;

import in3.JSON;
import in3.Proof;


public class ClientConfiguration implements Configuration {

    // Based on core/client/client_init.c
    private int requestCount = 1;
    private boolean autoUpdateList = true;
    private Proof proof = Proof.standard;
    private int maxAttempts = 3;
    private int signatureCount = 0;
    private int finality = 0;
    private boolean includeCode = false;
    private boolean keepIn3 = false;
    private boolean useBinary = false;
    private boolean useHttp = false;
    private long maxCodeCache = 0;
    private long timeout = 10000;
    private long minDeposit = 0;
    private long nodeProps = 0;
    private long nodeLimit = 0;
    private int replaceLatestBlock = 0;
    private String rpc;
    private long maxBlockCache = 0;

    private HashMap<Long, NodeConfiguration> nodesConfig = new HashMap<Long, NodeConfiguration>();

    public int getRequestCount() {
        return requestCount;
    }

    /** sets the number of requests send when getting a first answer */
    public void setRequestCount(int requestCount) {
        this.requestCount = requestCount;
    }

    public boolean isAutoUpdateList() {
        return autoUpdateList;
    }

    /**
     * activates the auto update.if true the nodelist will be automaticly updated if
     * the lastBlock is newer
     */
    public void setAutoUpdateList(boolean autoUpdateList) {
        this.autoUpdateList = autoUpdateList;
    }

    public Proof getProof() {
        return proof;
    }

    /** sets the type of proof used */
    public void setProof(Proof proof) {
        this.proof = proof;
    }

    public int getMaxAttempts() {
        return maxAttempts;
    }

    /** sets the max number of attempts before giving up */
    public void setMaxAttempts(int maxAttempts) {
        this.maxAttempts = maxAttempts;
    }

    public int getSignatureCount() {
        return signatureCount;
    }

    /** sets the number of signatures used to proof the blockhash. */
    public void setSignatureCount(int signatureCount) {
        this.signatureCount = signatureCount;
    }

    public int getFinality() {
        return finality;
    }

    /** sets the number of signatures in percent required for the request */
    public void setFinality(int finality) {
        this.finality = finality;
    }

    public boolean isIncludeCode() {
        return includeCode;
    }

    /* TODO ask what is this to Simon */
    public void setIncludeCode(boolean includeCode) {
        this.includeCode = includeCode;
    }

    public boolean isKeepIn3() {
        return keepIn3;
    }

    /* preserve in3 section of the rpc call response intact */
    public void setKeepIn3(boolean keepIn3) {
        this.keepIn3 = keepIn3;
    }

    public boolean isUseBinary() {
        return useBinary;
    }

    /* use binary payload instead of json */
    public void setUseBinary(boolean useBinary) {
        this.useBinary = useBinary;
    }

    public boolean isUseHttp() {
        return useHttp;
    }

    /* allow transport to use non-ssl */
    public void setUseHttp(boolean useHttp) {
        this.useHttp = useHttp;
    }

    public long getMaxCodeCache() {
        return maxCodeCache;
    }

    /** sets number of max bytes used to cache the code in memory */
    public void setMaxCodeCache(long maxCodeCache) {
        this.maxCodeCache = maxCodeCache;
    }

    public long getTimeout() {
        return timeout;
    }

    /**
     * specifies the number of milliseconds before the request times out. increasing
     * may be helpful if the device uses a slow connection.
     */
    public void setTimeout(long timeout) {
        this.timeout = timeout;
    }

    public long getMinDeposit() {
        return minDeposit;
    }

    /**
     * sets min stake of the server. Only nodes owning at least this amount will be
     * chosen.
     */
    public void setMinDeposit(long minDeposit) {
        this.minDeposit = minDeposit;
    }

    public long getNodeProps() {
        return nodeProps;
    }

    /* used to filter nodes according to its capabilities */
    public void setNodeProps(long nodeProps) {
        this.nodeProps = nodeProps;
    }

    public long getNodeLimit() {
        return nodeLimit;
    }

    /** sets the limit of nodes to store in the client. */
    public void setNodeLimit(long nodeLimit) {
        this.nodeLimit = nodeLimit;
    }

    public int getReplaceLatestBlock() {
        return replaceLatestBlock;
    }

    /** replaces the *latest* with blockNumber- specified value */
    public void setReplaceLatestBlock(int replaceLatestBlock) {
        this.replaceLatestBlock = replaceLatestBlock;
    }

    public String getRpc() {
        return rpc;
    }

    /** setup an custom rpc source for requests by setting Chain to local and proof to none */
    public void setRpc(String rpc) {
        this.rpc = rpc;
    }

    public long getMaxBlockCache() {
        return maxBlockCache;
    }

    /** sets the number of blocks cached in memory */
    public void setMaxBlockCache(long maxBlockCache) {
        this.maxBlockCache = maxBlockCache;
    }

    public HashMap<Long, NodeConfiguration> getNodesConfig() {
        return nodesConfig;
    }

    public void setNodesConfig(HashMap<Long, NodeConfiguration> nodesConfig) {
        this.nodesConfig = nodesConfig;
    }

    protected void addNodeConfiguration(NodeConfiguration configuration) {
        /*
         * This is stored in a HashMap to ensure uniqueness between chains without changing NodeConfiguration equals or toHash methods
         */
        nodesConfig.put(configuration.getChain(), configuration);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("{");

        JSON.appendKey(sb, "requestCount", getRequestCount());
        JSON.appendKey(sb, "autoUpdateList", isAutoUpdateList());
        JSON.appendKey(sb, "proof", JSON.asString(getProof()));
        JSON.appendKey(sb, "maxAttempts", getMaxAttempts());
        JSON.appendKey(sb, "signatureCount", getSignatureCount());
        JSON.appendKey(sb, "finality", getFinality());
        JSON.appendKey(sb, "includeCode", isIncludeCode());
        JSON.appendKey(sb, "keepIn3", isKeepIn3());
        JSON.appendKey(sb, "useBinary", isUseBinary());
        JSON.appendKey(sb, "useHttp", isUseHttp());
        JSON.appendKey(sb, "maxCodeCache", getMaxCodeCache());
        JSON.appendKey(sb, "timeout", getTimeout());
        JSON.appendKey(sb, "minDeposit", getMinDeposit());
        JSON.appendKey(sb, "nodeProps", getNodeProps());
        JSON.appendKey(sb, "nodeLimit", getNodeLimit());
        JSON.appendKey(sb, "replaceLatestBlock", getReplaceLatestBlock());
        if (getRpc() != null) {
            JSON.appendKey(sb, "rpc", getRpc());
        }
        JSON.appendKey(sb, "maxBlockCache", getMaxBlockCache());

        if (!nodesConfig.isEmpty()) {
            StringBuilder sb2 = new StringBuilder("{");
            for (NodeConfiguration nodeConfig: nodesConfig.values()) {
                JSON.appendKey(sb2, JSON.asString(nodeConfig.getChain()), nodeConfig);
            }

            sb2.setCharAt(sb2.length() - 1, '}');
            JSON.appendKey(sb, "nodes", sb2);
        }

        sb.setCharAt(sb.length() - 1, '}');
        return sb.toString();
    }

    @Override
    public String toJSON() {
        // TODO Auto-generated method stub
        return JSON.toJson(this);
    }
}
