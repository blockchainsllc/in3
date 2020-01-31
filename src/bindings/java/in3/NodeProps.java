package in3;

public final class NodeProps {
    /* filter out nodes which are providing no proof */
    public static final long NODE_PROP_PROOF            = 0x1;
    /* filter out nodes other then which have capability of the same RPC endpoint may also accept requests for different chains */
    public static final long NODE_PROP_MULTICHAIN       = 0x2;
    /* filter out non-archive supporting nodes */
    public static final long NODE_PROP_ARCHIVE          = 0x4;
    /* filter out non-http nodes  */
    public static final long NODE_PROP_HTTP             = 0x8;
    /* filter out nodes that don't support binary encoding */
    public static final long NODE_PROP_BINARY           = 0x10;
    /* filter out non-onion nodes */
    public static final long NODE_PROP_ONION            = 0x20;
    /* filter out non-signer nodes */
    public static final long NODE_PROP_SIGNER           = 0x40;
    /* filter out non-data provider nodes */
    public static final long NODE_PROP_DATA             = 0x80;
    /* filter out nodes that do not provide stats */
    public static final long NODE_PROP_STATS            = 0x100;
    /* filter out nodes that will sign blocks with lower min block height than specified */
    public static final long NODE_PROP_MIN_BLOCK_HEIGHT = 0x400;
}
