using In3;
using In3.Configuration;

namespace Test
{
    public class ClientBuilder
    {
        private IN3 _client;
        private Chain _chain;

        public ClientBuilder(Chain chain) {
            this._chain = chain;
        }

        public void CreateNewClient() {
            _client = IN3.ForChain(_chain);
        }

        public void BuildTransport(string[][] fileNameTuples) {
            StubTransport newTransport = new StubTransport();

            foreach (string[] fileNameTuple in fileNameTuples) {
                newTransport.AddMockedresponse(fileNameTuple[0], fileNameTuple[1]);
            }

            _client.Transport = newTransport;
        }

        public void BuildConfig()
        {
            ClientConfiguration clientConfig = _client.Configuration;

            ChainConfiguration mainNetConfiguration = new ChainConfiguration(Chain.Mainnet, clientConfig);
            mainNetConfiguration.NeedsUpdate = false;
            mainNetConfiguration.Contract = "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f";
            mainNetConfiguration.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";

            clientConfig.RequestCount = 1;
            clientConfig.AutoUpdateList = false;
            clientConfig.Proof = Proof.None;
            clientConfig.MaxAttempts = 1;
            clientConfig.SignatureCount = 0;
        }

        public IN3 ConstructClient(string[][] fileNameTuples) {
            CreateNewClient();
            BuildTransport(fileNameTuples);
            BuildConfig();
            return _client;
        }
    }
}