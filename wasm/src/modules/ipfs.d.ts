
/**
 * API for storing and retrieving IPFS-data.
 */
export declare interface IpfsAPI<BufferType> {
    /**
     * retrieves the content for a hash from IPFS.
     * @param multihash  the IPFS-hash to fetch
     *
     */
    get(multihash: string): Promise<BufferType>
    /**
     * stores the data on ipfs and returns the IPFS-Hash.
     * @param content puts a IPFS content
     */
    put(content: BufferType): Promise<string>
}
/*
public ipfs:IpfsAPI<BufferType>
 */