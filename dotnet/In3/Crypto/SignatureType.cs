namespace In3.Crypto
{
    /// <summary>
    /// Group of constants to be used along with the methods of <see cref="Crypto.Api" />.
    /// </summary>
    public class SignatureType
    {
        private SignatureType(string value) { Value = value; }

        internal string Value { get; set; }

        /// <summary>
        /// For hashes of the RLP prefixed.
        /// </summary>
        public static SignatureType EthSign { get { return new SignatureType("eth_sign"); } }

        /// <summary>
        /// For data that was signed directly.
        /// </summary>
        public static SignatureType Raw { get { return new SignatureType("raw"); } }

        /// <summary>
        /// For data that was hashed and then signed.
        /// </summary>
        public static SignatureType Hash { get { return new SignatureType("hash"); } }
    }
}