using System;

namespace In3.Configuration
{
    /// <summary>
    /// <see langword="Enum"/> that defines the capabilities an incubed node.
    /// </summary>
    [Flags] public enum Props
    {
        /// <summary>filter out nodes which are providing no proof.</summary>
        NodePropProof = 0x1,
        /// <summary>filter out nodes other then which have capability of the same RPC endpoint may also accept requests for different chains.</summary>
        NodePropMultichain = 0x2,
        /// <summary>filter out non-archive supporting nodes.</summary>
        NodePropArchive = 0x4,
        /// <summary>filter out non-http nodes.</summary>
        NodePropHttp = 0x8,
        /// <summary>filter out nodes that don't support binary encoding.</summary>
        NodePropBinary = 0x10,
        /// <summary>filter out non-onion nodes.</summary>
        NodePropOnion = 0x20,
        /// <summary>filter out non-signer nodes.</summary>
        NodePropSigner = 0x40,
        /// <summary>filter out non-data provider nodes.</summary>
        NodePropData = 0x80,
        /// <summary>filter out nodes that do not provide stats.</summary>
        NodePropStats = 0x100,
        /// <summary>filter out nodes that will sign blocks with lower min block height than specified.</summary>
        NodePropMinblockheight = 0x400,
    }
}