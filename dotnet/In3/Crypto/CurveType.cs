namespace In3.Crypto
{
    /// <summary>
    /// Enum to be used along with the methods of <see cref="Crypto.Signer" />.
    /// </summary>
    public enum CurveType : int
    {  
        /// <summary>Sign with ecdsa</summary>
        Ecdsa   = 1,
        /// <summary>Use ed25519 curve</summary>
        Ed25519 = 2,
    }
}