using In3;
using In3.Configuration;

namespace Test
{
    public class NoProofFactory : ClientFactory
    {
        public NoProofFactory(Chain chain) : base(chain)
        {
        }

        protected override void CreateConfig()
        {
            ClientConfiguration clientConfig = Client.Configuration;

            ChainConfiguration mainNetConfiguration = new ChainConfiguration(Chain.Mainnet, clientConfig);
            mainNetConfiguration.NeedsUpdate = false;
            mainNetConfiguration.Contract = "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f";
            mainNetConfiguration.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";

            ChainConfiguration ipfsConfiguration = new ChainConfiguration(Chain.Ipfs, clientConfig);
            ipfsConfiguration.NeedsUpdate = false;
            ipfsConfiguration.Contract = "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f";
            ipfsConfiguration.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";

            clientConfig.RequestCount = 1;
            clientConfig.AutoUpdateList = false;
            clientConfig.Proof = Proof.None;
            clientConfig.MaxAttempts = 10;
            clientConfig.SignatureCount = 0;
        }
    }
}