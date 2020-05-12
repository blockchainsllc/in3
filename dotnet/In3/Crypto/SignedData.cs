using System.Text.Json.Serialization;

namespace In3.Crypto
{
    /// <summary>
    /// Output of <see cref="Api.SignData" />.
    /// </summary>
    public class SignedData
    {
        /// <summary>
        /// Signed message.
        /// </summary>
        [JsonPropertyName("message")] public string Message { get; set; }

        /// <summary>
        /// Hash of (<see cref="Message" />.
        /// </summary>
        [JsonPropertyName("messageHash")] public string MessageHash { get; set; }
        /// <summary>
        /// ECDSA calculated r, s, and parity v, concatenated.
        /// </summary>
        [JsonPropertyName("signature")] public string Signature { get; set; }

        /// <summary>
        /// Part of the ECDSA signature.
        /// </summary>
        [JsonPropertyName("r")] public string R { get; set; }

        /// <summary>
        /// Part of the ECDSA signature.
        /// </summary>
        [JsonPropertyName("s")] public string S { get; set; }

        /// <summary>
        /// 27 + (<see cref="R" /> % 2).
        /// </summary>
        [JsonPropertyName("v")] public int V { get; set; }
    }
}