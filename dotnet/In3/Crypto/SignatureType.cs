namespace In3.Crypto
{
    public class SignatureType
    {
        private SignatureType(string value) { Value = value; }

        public string Value { get; set; }

        public static SignatureType EthSign   { get { return new SignatureType("eth_sign"); } }
        public static SignatureType Raw   { get { return new SignatureType("raw"); } }
        public static SignatureType Hash    { get { return new SignatureType("hash"); } }
    }
}