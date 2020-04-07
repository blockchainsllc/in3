namespace In3
{
    public enum Chain : uint
    {
        /**
         * support for multiple chains, a client can then switch between different chains (but consumes more memory)
         */
        Multichain = 0x0,
        /**
           * use mainnet
           */
        Mainnet = 0x1,
        /**
           * use kovan testnet
           */
        Kovan = 0x2a,
        /**
           * use tobalaba testnet
           */
        Tobalaba = 0x44d,
        /**
           * use goerli testnet
           */
        Goerli = 0x5,
        /**
           * use evan testnet
           */
        Evan = 0x4b1,
        /**
           * use ipfs
           */
        Ipfs = 0x7d0,
        /**
           * use volta test net
           */
        Volta = 0x12046,
        /**
           * use local client
           */
        Local = 0xFFFF
    }
}