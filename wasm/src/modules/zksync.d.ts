
export declare interface ZKAccountInfo {
    address: string,
    committed: {
        balances: {
            [name: string]: number
        },
        nonce: number,
        pubKeyHash: string
    },
    depositing: {
        balances: {
            [name: string]: number
        }
    },
    id: number,
    verified: {
        balances: {
            [name: string]: number
        },
        nonce: number,
        pubKeyHash: string
    }
}


/**
 * API for zksync.
 */
export declare interface ZksyncAPI<BufferType> {

    /**
     * gets current account Infoa and balances.
     * @param account the address of the account . if not specified, the first signer is used.
     */
    getAccountInfo(account?: string): Promise<ZKAccountInfo>
}


/**
 * zksync configuration.
 */
export declare interface zksync_config {
    /**
     * url of the zksync-server
     */
    provider_url?: string

    /**
    * the account to be used. if not specified, the first signer will be used.
    */
    account?: string

}


/*
public zksync:ZksyncAPI<BufferType>
 */