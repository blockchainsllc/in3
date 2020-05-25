namespace In3
{
    /// <summary>
    /// Defines the kind of entity associated with the ENS Resolved. Used along with <see cref="Eth1.Api.ENS" />.
    /// </summary>
    public enum ENSParameter
    {
        /// <summary>
        /// Address.
        /// </summary>
        Addr,
        /// <summary>
        /// Resolver.
        /// </summary>
        Resolver,
        /// <summary>
        /// Hash.
        /// </summary>
        Hash,
        /// <summary>
        /// Owner.
        /// </summary>
        Owner
    }
}