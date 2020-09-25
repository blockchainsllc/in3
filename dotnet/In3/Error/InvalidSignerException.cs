using System;
using In3.Crypto;

namespace In3.Error
{
    /// <summary>
    /// Exception thrown when there is no available <see cref="Signer"/> to sign.
    /// </summary>
    public class InvalidSignerException : Exception
    {
        internal InvalidSignerException(string message) : base(message) { }
    }
}