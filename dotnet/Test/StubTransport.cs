using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using In3.Transport;
using Newtonsoft.Json.Linq;

namespace Test
{
    public class StubTransport : Transport
    {
        private Dictionary<string, string> Responses { get; }
        private const string ResponsesDir = "Responses";

        public StubTransport()
        {
            Responses = new Dictionary<string, string>();
        }

        public Task<string> Handle(string url, string payload)
        {
            return Task.Run(() =>  Responses[GetMethod(payload)]);
        }

        public void AddMockedresponse(string methodName, string filename)
        {
            string content = ReadFile(filename);
            Responses.Add(methodName, content);
        }

        private string GetMethod(string payload)
        {
            var payloadObj = JArray.Parse(payload);
            return payloadObj.First["method"].ToString();
        }

        private string ReadFile(string filename)
        {
            string fullPath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), ResponsesDir, filename);
            return File.ReadAllText(fullPath);
        }
    }
}