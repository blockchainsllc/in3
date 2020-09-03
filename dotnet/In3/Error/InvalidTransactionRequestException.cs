using System;
using In3.Eth1;

namespace In3.Error
{
    /// <summary>
    /// Exception thrown when the state of <see cref="TransactionRequest"/> is invalid.
    /// </summary>
    public class InvalidTransactionRequestException : Exception
    {
        internal InvalidTransactionRequestException(string message)
            : base(message)
        {
        }
    }
}