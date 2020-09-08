
/**
 * API for zksync.
 */
export declare interface ZksyncAPI<BufferType> {
}


/**
 * zksync configuration.
 */
export declare interface zksync_config {
    /**
     * max number of DAPs (Difficulty Adjustment Periods) allowed when accepting new targets.
     */
    maxDAP?: number

    /**
    * max increase (in percent) of the difference between targets when accepting new targets.
    */
    maxDiff?: number

}


/*
public zksync:ZksyncAPI<BufferType>
 */