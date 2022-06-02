namespace In3
{
    /// <summary>Represents the multiple chains supported by Incubed.</summary>
    public enum Chain : uint
    {        /// <summary>Ethereum mainnet.</summary>
        Mainnet = 0x1,

        /// <summary>Tobalaba testnet.</summary>
        Tobalaba = 0x44d,

        /// <summary>Goerli testnet.</summary>
        Goerli = 0x5,

        /// <summary>Mumbai testnet.</summary>
        Mumbai = 0x13881,

        /// <summary>Bitcoin chain.</summary>
        Btc = 0x99,

        /// <summary>Ewf chain.</summary>
        Ewc = 0xf6,

        /// <summary>Ipfs (InterPlanetary File System).</summary>
        Ipfs = 0x7d0,

        /// <summary>Volta testnet.</summary>
        Volta = 0x12046,

        /// <summary>Local client.</summary>
        Local = 0xFFFF
    }
}