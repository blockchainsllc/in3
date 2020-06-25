using System.Numerics;

namespace In3.Eth1
{
    /// <summary>
    /// Class that holds the state for the transaction request to be submited via <see cref="Eth1.Api.SendTransaction" />.
    /// </summary>
    public class TransactionRequest
    {
        /// <summary>
        /// Address derivated from the private key that will sign the transaction. See <see cref="Crypto.Signer" />.
        /// </summary>
        public string From { get; set; }

        /// <summary>
        /// Address to whom the transaction value will be transfered to or the smart contract address whose function will be invoked.
        /// </summary>
        public string To { get; set; }

        /// <summary>
        /// Data of the transaction (in the case of a smart contract deployment for exemple).
        /// </summary>
        public string Data { get; set; }

        /// <summary>
        /// Value of the transaction.
        /// </summary>
        public BigInteger? Value { get; set; }

        /// <summary>
        /// Nonce of the transaction.
        /// </summary>
        public long? Nonce { get; set; }

        /// <summary>
        /// Gas cost for the transaction. Can be estimated via <see cref="Eth1.Api.EstimateGas" />.
        /// </summary>
        public long? Gas { get; set; }

        /// <summary>
        /// Gas price (in wei). Can be obtained via <see cref="Eth1.Api.GetGasPrice" />.
        /// </summary>
        public long? GasPrice { get; set; }

        /// <summary>
        /// Function of the smart contract to be invoked.
        /// </summary>
        public string Function { get; set; }

        /// <summary>
        /// Array of parameters for the function (in the same order of its signature), see <see cref="Function" />
        /// </summary>
        public object[] Params { get; set; }

        internal bool IsFunctionInvocation() => !string.IsNullOrEmpty(Function);
    }
}