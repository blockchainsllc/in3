using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;

namespace In3.Transport
{
    /// <summary>
    /// Basic implementation for synchronous http transport for Incubed client.
    /// </summary>
    public class DefaultTransport : Transport
    {
        private readonly HttpClient client = new HttpClient();

        /// <summary>
        /// Standard construction.
        /// </summary>
        public DefaultTransport()
        {
            client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
        }

        /// <summary>
        /// Method that handles, sychronously the http requests.
        /// </summary>
        /// <param name="url">The url of the node.</param>
        /// <param name="payload">Json for the body of the POST request to the node.</param>
        /// <returns>The http json response.</returns>
        public string Handle(string url, string payload)
        {
            var httpWebRequest = (HttpWebRequest)WebRequest.Create(url);
            httpWebRequest.ContentType = "application/json";
            httpWebRequest.Method = "POST";
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls13 | SecurityProtocolType.Tls12 | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls;

            using (var streamWriter = new StreamWriter(httpWebRequest.GetRequestStream()))
            {
                streamWriter.Write(payload);
                streamWriter.Flush();
                streamWriter.Close();
            }

            var httpResponse = (HttpWebResponse)httpWebRequest.GetResponse();
            using (var streamReader = new StreamReader(httpResponse.GetResponseStream()))
            {
                return streamReader.ReadToEnd();
            }
        }
    }
}