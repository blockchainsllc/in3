using System;

namespace In3.Exceptions
{
    /// <summary>
    /// Custom Exception to be thrown during the 
    /// </summary>
    public class RpcException : Exception
    {
        internal RpcException()
        {
        }

        internal RpcException(string message)
            : base(message)
        {
        }

        internal RpcException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}