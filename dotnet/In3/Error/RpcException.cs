using System;

namespace In3.Error
{
    /// <summary>
    /// Custom Exception to be thrown in case of Rpc errors.
    /// </summary>
    public class RpcException : In3Exception
    {
        /// <summary>
        /// Json-rpc code for the error.
        /// </summary>
        public int Code { get; }
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

        internal RpcException(int code, string message)
            : base(message)
        {
            Code = code;
        }
    }
}