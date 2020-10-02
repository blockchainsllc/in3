using In3;
using In3.Configuration;

namespace Test
{
    public abstract class ClientFactory
    {
        protected IN3 Client;
        private readonly Chain _chain;

        protected ClientFactory(Chain chain)
        {
            this._chain = chain;
        }

        private void CreateClient()
        {
            Client = IN3.ForChain(_chain);
        }

        protected abstract void CreateConfig();

        private void CreateTransport(string[][] fileNameTuples)
        {
            StubTransport newTransport = new StubTransport();
            foreach (string[] fileNameTuple in fileNameTuples)
            {
                newTransport.AddMockedresponse(fileNameTuple[0], fileNameTuple[1]);
            }

            Client.Transport = newTransport;
        }

        public IN3 CreateIn3(string[][] fileNameTuples)
        {
            CreateClient();
            CreateConfig();
            CreateTransport(fileNameTuples);
            return Client;
        }
    }
}