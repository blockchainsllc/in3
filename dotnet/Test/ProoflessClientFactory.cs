using In3;
using In3.Configuration;

namespace Test
{
    public class ProoflessClientFactory : ClientFactory
    {
        public ProoflessClientFactory(Chain chain) : base(chain)
        {
        }

        protected override void CreateConfig()
        {
            ClientConfiguration clientConfig = Client.Configuration;

            NodeRegistryConfiguration currentChainConfiguration = clientConfig.NodeRegistry;
            currentChainConfiguration.NeedsUpdate = false;
            currentChainConfiguration.Contract = "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f";
            currentChainConfiguration.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";

            clientConfig.RequestCount = 1;
            clientConfig.AutoUpdateList = false;
            clientConfig.Proof = Proof.None;
            clientConfig.MaxAttempts = 10;
            clientConfig.SignatureCount = 0;
        }
    }
}