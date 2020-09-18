using System;

namespace In3.Error
{
    /// <summary>
    /// Exception thrown when there is an issue with the Context engine.
    /// </summary>
    public class ContextException : Exception
    {
        internal ContextException(string message) : base(message)
        {
        }

        internal ContextException(string message, Exception inner) : base(message, inner)
        {
        }
    }
}