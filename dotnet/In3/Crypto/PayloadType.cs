namespace In3.Crypto
{
    /// <summary>
    /// Enum to be used along with the methods of <see cref="Crypto.Signer" />.
    /// </summary>
    public enum PayloadType : int
    {
        /// <summary>Custom data to be signed</summary>
        Any    = 0,
        /// <summary>The payload is a ethereum-tx</summary>
        EthTx  = 1,
        /// <summary> The payload is a BTC-Tx-Input</summary>
        BtcTx  = 2,
        /// <summary> The payload is a rlp-encoded data of a Gnosys Safe Tx</summary>
        SafeTx = 3
    }
}