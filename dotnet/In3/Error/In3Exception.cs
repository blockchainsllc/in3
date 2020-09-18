using System;

namespace In3.Error
{
    /// <summary>
    /// General purpose, all-encompassing, Incubed specific Exception class.
    /// </summary>
    public abstract class In3Exception : Exception
    {
        internal In3Exception()
        {
        }

        internal In3Exception(string message)
            : base(message)
        {
        }

        internal In3Exception(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}