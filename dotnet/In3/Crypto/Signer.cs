using System.Threading.Tasks;

namespace In3.Crypto
{
    /// <summary>
    /// Minimum interface to be implemented by a kind of signer. Used by <see cref="Eth1.Api.SendTransaction" />. Set it with <see cref="IN3.Signer" />.
    /// </summary>
    public interface Signer
    {
        /// <summary>
        /// Queries the Signer if it can sign for a certain key.
        /// </summary>
        /// <param name="account">The account derived from the private key used to sign transactions.</param>
        /// <returns><see langword="true"/> if it can sign, <see langword="false"/> if it cant.</returns>
        /// <remarks>This method is invoked internaly by <see cref="Eth1.Api.SendTransaction" /> using <see cref="TransactionRequest.From" /> and will throw a <see langword="SystemException" /> in case <see langword="false"/> is returned.</remarks>
        bool CanSign(string account);

        /// <summary>
        /// Signs the transaction data with the private key associated with the invoked account. Both arguments are automaticaly passed by Incubed client base on <see cref="TransactionRequest" /> data during a <see cref="Eth1.Api.SendTransaction" />.
        /// </summary>
        /// <param name="data">Data to be signed.</param>
        /// <param name="account">The account that will sign the transaction.</param>
        /// <param name="digestType">The digest type used for signing.</param>
        /// <param name="payloadType">The payload type, used for deserialization.</param>
        /// <param name="curveType">The type of the curve.</param>
        /// <returns>The signed transaction data.</returns>
        Task<string> Sign(string data, string account, DigestType digestType, PayloadType payloadType, CurveType curveType);
    }
}