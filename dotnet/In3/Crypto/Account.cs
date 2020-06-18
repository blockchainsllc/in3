using System.Text.Json.Serialization;

namespace In3.Crypto
{
    /// <summary>
    /// Composite entity that holds address and public key. It represents and Ethereum acount. Entity returned from <see cref="Crypto.Api.EcRecover" />.
    /// </summary>
    public class Account
    {
        /// <summary>
        /// The address.
        /// </summary>
        [JsonPropertyName("address")] public string Address { get; set; }

        /// <summary>
        /// The public key.
        /// </summary>
        [JsonPropertyName("publicKey")] public string PublicKey { get; set; }
    }
}