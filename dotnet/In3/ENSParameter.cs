using In3.Eth1;

namespace In3
{
    /// <summary>
    /// Defines the kind of entity associated with the ENS Resolved. Used along with <see cref="Api.Ens" />.
    /// </summary>
    public class ENSParameter
    {
        private ENSParameter(string value) { Value = value; }

        internal string Value { get; }

        /// <summary>
        /// Address.
        /// </summary>
        public static ENSParameter Addr => new ENSParameter("addr");

        /// <summary>
        /// Resolver.
        /// </summary>
        public static ENSParameter Resolver => new ENSParameter("resolver");
        /// <summary>
        /// Hash.
        /// </summary>
        public static ENSParameter Hash => new ENSParameter("hash");
        /// <summary>
        /// Owner.
        /// </summary>
        public static ENSParameter Owner => new ENSParameter("owner");
    }
}