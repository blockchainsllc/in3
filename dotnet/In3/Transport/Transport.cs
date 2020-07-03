using System.Threading.Tasks;

namespace In3.Transport
{
    /// <summary>
    /// Minimum interface for a custom transport. Transport is a mean of communication with the Incubed server.
    /// </summary>
    public interface Transport
    {
        /// <summary>
        /// Method to be implemented that will handle the requests to the server. This method may be called once for each url on each batch of requests.
        /// </summary>
        /// <param name="url">Url of the node.</param>
        /// <param name="payload">Content for the RPC request.</param>
        /// <returns>The rpc response.</returns>
        public Task<string> Handle(string url, string payload);
    }
}