using System.Collections.Generic;

namespace In3.Configuration
{
    /// <summary>
    /// Alias for verification levels. Verification is done by calculating Ethereum Trie states requested by the Incubed network ans signed as proofs of a certain state.
    /// </summary>
    public class Proof
    {
        private Proof(string value) { Value = value; }
        private static Dictionary<string, Proof> _valueToProof = new Dictionary<string, Proof> {
            {"none", None},
            {"standard", Standard},
            {"full", Full}
        };

        internal static Proof FromValue(string value) => _valueToProof.ContainsKey(value) ? _valueToProof[value] : null;
        internal string Value { get; set; }

        /// <summary>
        /// No Verification.
        /// </summary>
        public static Proof None { get { return new Proof("none"); } }

        /// <summary>
        /// Standard Verification of the important properties.
        /// </summary>
        public static Proof Standard { get { return new Proof("standard"); } }

        /// <summary>
        /// All fields will be validated (including uncles).
        /// </summary>
        public static Proof Full { get { return new Proof("full"); } }
    }
}