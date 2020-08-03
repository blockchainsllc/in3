namespace In3
{
    /// <summary>Represents the multiple chains supported by Incubed.</summary>
    public enum Chain : uint
    {
        /// <summary>Support for multiple chains, a client can then switch between different chains (but consumes more memory).</summary>
        Multichain = 0x0,

        /// <summary>Ethereum mainnet.</summary>
        Mainnet = 0x1,

        /// <summary>Kovan testnet.</summary>
        Kovan = 0x2a,

        /// <summary>Tobalaba testnet.</summary>
        Tobalaba = 0x44d,

        /// <summary>Goerli testnet.</summary>
        Goerli = 0x5,

        /// <summary>Bitcoin chain.</summary>
        Btc = 0x99,

        /// <summary>Ewf chain.</summary>
        Ewc = 0xf6,

        /// <summary>Evan testnet.</summary>
        Evan = 0x4b1,

        /// <summary>Ipfs (InterPlanetary File System).</summary>
        Ipfs = 0x7d0,

        /// <summary>Volta testnet.</summary>
        Volta = 0x12046,

        /// <summary>Local client.</summary>
        Local = 0xFFFF
    }
}