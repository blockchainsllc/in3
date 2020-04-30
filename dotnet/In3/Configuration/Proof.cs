using System.Collections.Generic;

namespace In3.Configuration
{
    public class Proof
    {
        private Proof(string value) { Value = value; }
        private static Dictionary<string, Proof> _valueToProof = new Dictionary<string, Proof> {
            {"none", None},
            {"standard", Standard},
            {"full", Full}
        };

        public static Proof FromValue(string value) => _valueToProof.ContainsKey(value) ? _valueToProof[value] : null;
        public string Value { get; set; }

        public static Proof None { get { return new Proof("none"); } }
        public static Proof Standard { get { return new Proof("standard"); } }
        public static Proof Full { get { return new Proof("full"); } }
    }
}