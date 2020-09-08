
/**
 * a Input of a Bitcoin Transaction
 */
export declare interface BtcTransactionInput {
    /** the transaction id  */
    txid: Hash

    /** the index of the transactionoutput */
    vout: number

    /** the script */
    scriptSig: {
        /** the asm data */
        asm: Data

        /** the raw hex data */
        hex: Data
    }

    /**  The script sequence number */
    sequence: number

    /** hex-encoded witness data (if any) */
    txinwitness: Data[]
}
/**
 * a Input of a Bitcoin Transaction
 */
export declare interface BtcTransactionOutput {
    /** the value in BTC  */
    value: number

    /** the index */
    n: number

    /** the index of the transactionoutput */
    vout: number

    /** the script */
    scriptPubKey: {
        /** the asm data */
        asm: Data

        /** the raw hex data */
        hex: Data

        /** the required sigs */
        reqSigs: number

        /** The type, eg 'pubkeyhash' */
        type: string

        /** list of addresses */
        addresses: Address[]
    }
}

/**
 * a BitCoin Transaction.
 */
export declare interface BtcTransaction {
    /** true if this transaction is part of the longest chain */
    in_active_chain: boolean

    /** the hex representation of raw data*/
    hex: Data

    /** The requested transaction id. */
    txid: Hash

    /** The transaction hash (differs from txid for witness transactions) */
    hash: Hash

    /** The serialized transaction size */
    size: number

    /** The virtual transaction size (differs from size for witness transactions) */
    vsize: number

    /** The transaction’s weight (between vsize4-3 and vsize4) */
    weight: number

    /** The version */
    version: number

    /** The locktime */
    locktime: number

    /** the block hash of the block containing this transaction. */
    blockhash: Hash

    /** The confirmations. */
    confirmations: number

    /** The transaction time in seconds since epoch (Jan 1 1970 GMT) */
    time: number

    /** The block time in seconds since epoch (Jan 1 1970 GMT) */
    blocktime: number

    /** the transaction inputs */
    vin: BtcTransactionInput[]

    /** the transaction outputs */
    vout: BtcTransactionOutput[]

}
/** a Block header */
export interface BTCBlockHeader {
    /** the hash of the blockheader */
    hash: string,
    /** number of confirmations or blocks mined on top of the containing block*/
    confirmations: number,
    /** block number */
    height: number,
    /** used version  */
    version: number,
    /**  version as hex */
    versionHex: string,
    /** merkle root of the trie of all transactions in the block  */
    merkleroot: string,
    /** unix timestamp in seconds since 1970 */
    time: string,
    /** unix timestamp in seconds since 1970 */
    mediantime: string,
    /** nonce-field of the block */
    nonce: number,
    /** bits (target) for the block as hex*/
    bits: string,
    /** difficulty of the block */
    difficulty: number,
    /** total amount of work since genesis */
    chainwork: string,
    /**  number of transactions in the block  */
    nTx: number,
    /**  hash of the parent blockheader  */
    previousblockhash: string,
    /**  hash of the next blockheader  */
    nextblockhash: string
}

/** a full Block including the transactions */
export interface BTCBlock<T> extends BTCBlockHeader {
    /** the transactions */
    tx: T[]
}


/**
 * API for handling BitCoin data
 */
export declare interface BtcAPI<BufferType> {
    /** retrieves the transaction and returns the data as json. */
    getTransaction(txid: Hash): Promise<BtcTransaction>

    /** retrieves the serialized transaction (bytes) */
    getTransactionBytes(txid: Hash): Promise<BufferType>

    /** retrieves the blockheader and returns the data as json. */
    getBlockHeader(blockHash: Hash): Promise<BTCBlockHeader>

    /** retrieves the serialized blockheader (bytes) */
    getBlockHeaderBytes(blockHash: Hash): Promise<BufferType>

    /** retrieves the block including all tx data as json. */
    getBlockWithTxData(blockHash: Hash): Promise<BTCBlock<BtcTransaction>>

    /** retrieves the block including all tx ids as json. */
    getBlockWithTxIds(blockHash: Hash): Promise<BTCBlock<string>>

    /** retrieves the serialized block (bytes) including all transactions */
    getBlockBytes(blockHash: Hash): Promise<BufferType>
}

/**
 * bitcoin configuration.
 */
export declare interface btc_config {
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
public btc:BtcAPI<BufferType>
 */