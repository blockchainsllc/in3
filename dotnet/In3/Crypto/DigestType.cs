namespace In3.Crypto
{
    /// <summary>
    /// Enum to be used along with the methods of <see cref="Crypto.Signer" />.
    /// </summary>
    public enum DigestType : int
    {
        /// <summary>sign the data directly</summary>
        Raw    = 0,
        /// <summary>hash and sign the data</summary>
        Hash   = 1,
        /// <summary>add Ethereum Signed Message-Proefix, hash and sign the data</summary>
        Prefix = 2,
        /// <summary>hashes the data twice with sha256 and signs it</summary>
        Btc    = 3
    }
}